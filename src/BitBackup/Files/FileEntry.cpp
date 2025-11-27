/*
 * MIT License
 * Copyright (c) 2023-2025 Robert Vokac
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/**
 *
 * @author robertvokac
 */

#include <algorithm>

#include "BitBackup/Files/FileType.h"
#include "BitBackup/Files/UnixPermissions.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstring>
#include <vector>
#include <map>
#include <string>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include "BitBackup/Core/Utils.h"
#include <sys/xattr.h>
#include "BitBackup/Files/FileEntry.h"


namespace BitBackup::Files
{
    namespace fs = std::filesystem;
    using std::string;


    FileEntry::FileEntry(const string& filePath)
    {
        std::filesystem::path fPath(filePath);
        fileName = fPath.filename().string();
        {
            size_t pos;
            while ((pos = fileName.find("\t")) != std::string::npos)
            {
                fileName.replace(pos, 1, " ");
            }
        }

        path = fileName == SLASH ? "" : fPath.parent_path().string();

        auto fStatus = std::filesystem::status(fPath);

        fileType = FileTypeHelper::forFile(fStatus);
        linkTarget = fileType == FileType::LINK ? std::filesystem::read_symlink(fPath).string() : "";

        struct stat fileStat;
        if (stat(filePath.c_str(), &fileStat) == 0)
        {
            uid = fileStat.st_uid;
            gid = fileStat.st_gid;
            creationTime = fileStat.st_ctime;
            lastModTime = fileStat.st_mtime;
            lastAccessTime = fileStat.st_atime;
            lastChangeTime = fileStat.st_ctime;
        }

        struct passwd* pw = getpwuid(uid);
        struct group* gr = getgrgid(gid);
        owner = pw ? pw->pw_name : "Unknown";
        group = gr ? gr->gr_name : "Unknown";

        fs::file_status status = fs::status(filePath);
        auto permissions = status.permissions();

        std::cout << "User: " << ((permissions & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
            << ((permissions & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
            << ((permissions & fs::perms::owner_exec) != fs::perms::none ? "x" : "-") << std::endl;

        std::cout << "Group: " << ((permissions & fs::perms::group_read) != fs::perms::none ? "r" : "-")
            << ((permissions & fs::perms::group_write) != fs::perms::none ? "w" : "-")
            << ((permissions & fs::perms::group_exec) != fs::perms::none ? "x" : "-") << std::endl;

        std::cout << "Others: " << ((permissions & fs::perms::others_read) != fs::perms::none ? "r" : "-")
            << ((permissions & fs::perms::others_write) != fs::perms::none ? "w" : "-")
            << ((permissions & fs::perms::others_exec) != fs::perms::none ? "x" : "-") << std::endl;


        bool userR = (permissions & fs::perms::owner_read) != fs::perms::none;
        bool userW = (permissions & fs::perms::owner_write) != fs::perms::none;
        bool userX = (permissions & fs::perms::owner_exec) != fs::perms::none;
        //
        bool groupR = (permissions & fs::perms::group_read) != fs::perms::none;
        bool groupW = (permissions & fs::perms::group_write) != fs::perms::none;
        bool groupX = (permissions & fs::perms::group_exec) != fs::perms::none;
        //
        bool othersR = (permissions & fs::perms::others_read) != fs::perms::none;
        bool othersW = (permissions & fs::perms::others_write) != fs::perms::none;
        bool othersX = (permissions & fs::perms::others_exec) != fs::perms::none;
        unixPermissions.getUser().setRead(userR);
        unixPermissions.getUser().setWrite(userW);
        unixPermissions.getUser().setExecute(userX);
        //
        unixPermissions.getGroup().setRead(groupR);
        unixPermissions.getGroup().setWrite(groupW);
        unixPermissions.getGroup().setExecute(groupX);
        //
        unixPermissions.getOthers().setRead(othersR);
        unixPermissions.getOthers().setWrite(othersW);
        unixPermissions.getOthers().setExecute(othersX);
        //

        size = is_directory(fPath) ? 0 : std::filesystem::file_size(fPath);
        hashAlgorithm = SH_A256;
        hashSum = BitBackup::Core::Utils::calculateSHA256Hash(fPath);


        ssize_t size = listxattr(filePath.c_str(), nullptr, 0);

        if (size > 0)
        {
            for (const auto& [key, value] : getUserDefinedAttributes(fPath))
            {
                attrs.insert({key, value});
            }
        }
    }

    std::map<std::string, std::string> FileEntry::getUserDefinedAttributes(const std::string& filePath) const
    {
        std::map<std::string, std::string> attrs;


        ssize_t size = listxattr(filePath.c_str(), nullptr, 0);
        if (size <= 0) return attrs;

        std::vector<char> buffer(size);
        size = listxattr(filePath.c_str(), buffer.data(), buffer.size());

        char* key = buffer.data();
        while (key < buffer.data() + size)
        {
            ssize_t valueSize = getxattr(filePath.c_str(), key, nullptr, 0);
            std::vector<char> valueBuffer(valueSize);
            getxattr(filePath.c_str(), key, valueBuffer.data(), valueBuffer.size());

            attrs[key] = std::string(valueBuffer.begin(), valueBuffer.end());
            key += strlen(key) + 1;
        }

        return attrs;
    }

    string FileEntry::attrsAsBase64EncodedString() const
    {
        std::stringstream attrsSb;

        std::vector<std::string> keysList;

        for (const auto& pair : attrs)
        {
            keysList.push_back(pair.first);
        }


        std::sort(keysList.begin(), keysList.end());

        for (const auto& key : keysList)
        {
            attrsSb << key << "=" << attrs.at(key) << "\n";
        }

        return Core::Utils::encodeBase64(attrsSb.str());
    }

    string FileEntry::toCsvLine() const
    {
        std::stringstream sb;
        sb << path << '\t' << fileName << '\t' << FileTypeHelper::toChar(fileType) << '\t'
            << linkTarget << '\t' << uid << '\t' << gid << '\t'
            << owner << '\t' << group << '\t' << unixPermissions.toString() << '\t'
            << creationTime << '\t' << lastModTime << '\t' << lastChangeTime << '\t'
            << lastAccessTime << '\t' << size << '\t' << hashAlgorithm << '\t'
            << hashSum << '\t' << (attrs.empty() ? "" : attrsAsBase64EncodedString());

        return sb.str();
    }

    // public static void main(String[] args) throws IOException {
    //     final FileEntry fileEntry = new FileEntry(new File("/home/robertvokac/Downloads/zim_0.75.1-1_all.deb"));
    //     System.out.println(fileEntry.toCsvLine());
    // }
    ;
}
