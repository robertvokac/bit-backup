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


// import dev.mccue.guava.hash.Hashing;
// import dev.mccue.guava.io.Files;
// import java.io.BufferedReader;
// import java.io.File;
// import java.io.FileWriter;
// import java.io.IOException;
// import java.io.InputStream;
// import java.io.InputStreamReader;
// import java.io.PrintWriter;
// import java.nio.file.Path;
// import java.nio.file.Paths;
// import java.nio.file.StandardCopyOption;
//
// import java.util.ArrayList;
// import java.util.Base64;
// import java.util.List;
// import java.util.logging.Level;
// import java.util.logging.Logger;

#include "BitBackup/Core/Utils.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <vector>

#include "BitBackup/Core/BitBackupException.h"
#include <openssl/sha.h> // Requires OpenSSL library for hashing
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace BitBackup::Core {


    std::vector<std::filesystem::path> Utils::listAllFilesInDir(const std::filesystem::path& dir){
        std::vector<std::filesystem::path> files;
        listAllFilesInDir(dir, files);
        return files;
    }

    std::vector<std::filesystem::path> Utils::listAllFilesInDir(const std::filesystem::path& dir, std::vector<std::filesystem::path>& files) {

        files.push_back(dir);
        for (const auto &f : std::filesystem::directory_iterator(dir)) {
            if (f.is_directory()) {
                listAllFilesInDir(f, files);
            } else {
                files.push_back(f);
            }
        }
        return files;
    }

    void Utils::writeTextToFile(const std::string &text, const std::filesystem::path& file) {

        try {
            std::ofstream fileWriter(file);
            if (!fileWriter.is_open()) {
                throw BitBackupException("Writing to file failed: " + file.filename().string());
            }
            fileWriter << text;
            fileWriter.close();
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            throw BitBackupException("Writing to file failed: " + file.filename().string());
        }
    }
//apt-get install libssl-dev
    std::string Utils::readTextFromFile(const std::filesystem::path& filePath) {
        if (!std::filesystem::exists(filePath)) {
            return ""; // Return an empty string if the file does not exist
        }

        try {
            std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
            if (!fileStream) {
                throw BitBackupException("Reading file failed: " + filePath.string());
            }

            std::ostringstream buffer;
            buffer << fileStream.rdbuf();
            return buffer.str(); // Return the file contents as a string
        } catch (const std::exception& ex) {
            throw BitBackupException("Reading file failed: " + filePath.string() + ", " + ex.what());
        }
    }

    enum SHAALG {SHA512, SHA256};
    std::string calculateSHAHash(const std::filesystem::path& filePath, SHAALG algorithm) {
        switch (algorithm) {
            case SHA512: break;
            case SHA256: break;
            default: throw BitBackupException("Unsupported algorithm: " + std::to_string(algorithm));
        }
        if (!std::filesystem::exists(filePath)) {
            throw BitBackupException("File does not exist: " + filePath.string());
        }

        try {
            std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
            if (!fileStream) {
                throw BitBackupException("Failed to open file: " + filePath.string());
            }

            union {
                SHA512_CTX sha512Ctx;
                SHA256_CTX sha256Ctx;
            } sha_context;

            if (algorithm == SHA512) {
                SHA512_Init(&(sha_context.sha512Ctx));
            }

            if (algorithm == SHA256) {
                SHA256_Init(&(sha_context.sha256Ctx));
            }

            // Read in large chunks (heap-allocated) to cut the number of read()
            // syscalls on big files. 1 MiB is a good throughput/footprint tradeoff.
            constexpr std::size_t bufferSize = 1u << 20; // 1 MiB
            std::vector<char> buffer(bufferSize);
            while (fileStream.read(buffer.data(), static_cast<std::streamsize>(bufferSize))) {
                if (algorithm == SHA512) {
                    SHA512_Update(&(sha_context.sha512Ctx), buffer.data(), fileStream.gcount());
                }

                if (algorithm == SHA256) {
                    SHA256_Update(&(sha_context.sha256Ctx), buffer.data(), fileStream.gcount());
                }

            }
            // Handle any remaining bytes at the end of the file
            if (fileStream.gcount() > 0) {
                if (algorithm == SHA512) {
                    SHA512_Update(&(sha_context.sha512Ctx), buffer.data(), fileStream.gcount());
                }

                if (algorithm == SHA256) {
                    SHA256_Update(&(sha_context.sha256Ctx), buffer.data(), fileStream.gcount());
                }

            }

            int SHAXXX_DIGEST_LENGTH = 0;
            if (algorithm == SHA512) {
                SHAXXX_DIGEST_LENGTH = SHA512_DIGEST_LENGTH;
            }

            if (algorithm == SHA256) {
                SHAXXX_DIGEST_LENGTH = SHA256_DIGEST_LENGTH;
            }
            if (SHAXXX_DIGEST_LENGTH == 0) {
                throw BitBackupException("SHA digest length mismatch.");
            }

            unsigned char hash512[SHA512_DIGEST_LENGTH];
            unsigned char hash256[SHA256_DIGEST_LENGTH];

            if (algorithm == SHA512) {
                SHA512_Final(hash512, &(sha_context.sha512Ctx));
            }

            if (algorithm == SHA256) {
                SHA256_Final(hash256, &(sha_context.sha256Ctx));
            }

            // Convert hash to a hexadecimal string
            std::ostringstream hashStream;
            hashStream << std::hex; // Set hexadecimal format
            for (size_t i = 0; i < SHAXXX_DIGEST_LENGTH; ++i) {
                hashStream << std::setw(2) << std::setfill('0') << static_cast<int>(
                    algorithm == SHA512 ? hash512[i] : hash256[i]
                    );
            }
            hashStream << std::dec; // Reset to decimal format
            return hashStream.str();
        } catch (const std::exception& ex) {
            throw BitBackupException("Hashing file failed: " + filePath.string() + ", " + ex.what());
        }
    }

    std::string Utils::calculateSHA512Hash(const std::filesystem::path& filePath) {
        return calculateSHAHash(filePath, SHA512);
    }
    std::string Utils::calculateSHA256Hash(const std::filesystem::path& filePath) {
        return calculateSHAHash(filePath, SHA256);
    }


    std::string Utils::createJdbcUrl(const std::string& directoryWhereSqliteFileIs) {
        return "jdbc:sqlite:" + directoryWhereSqliteFileIs + "/" + ".bitbackup.sqlite3?foreign_keys=on;";
    }

    std::string Utils::encodeBase64(const std::string &input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, input.data(), static_cast<int>(input.size()));
        if (BIO_flush(bio) != 1) {
            BIO_free_all(bio);
            throw std::runtime_error("Failed to flush BIO during Base64 encoding.");
        }

        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string encoded(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);

        return encoded;
    }


    std::string Utils::decodeBase64(const std::string &input) {
        BIO *bio = nullptr, *b64 = nullptr;
        char inbuf[512];
        std::string output;
        int inlen;

        b64 = BIO_new(BIO_f_base64());
        if (!b64)
            throw std::runtime_error("Failed to create base64 BIO.");

        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

        bio = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
        if (!bio) {
            BIO_free(b64);
            throw std::runtime_error("Failed to create memory BIO.");
        }

        bio = BIO_push(b64, bio);

        while ((inlen = BIO_read(bio, inbuf, sizeof(inbuf))) > 0) {
            output.append(inbuf, inlen);
        }

        if (inlen < 0) {
            BIO_free_all(bio);
            throw std::runtime_error("Error while reading from BIO during Base64 decoding.");
        }

        BIO_free_all(bio);
        return output;
    }

    std::string Utils::generateUUIDv4() {
        static std::mt19937 gen = []() {
            std::random_device rd;
            std::seed_seq seq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
            return std::mt19937(seq);
        }();
        static std::mutex genMutex;
        std::lock_guard<std::mutex> lock(genMutex);
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        ss << std::setw(8) << dist(gen) << "-";
        ss << std::setw(4) << (dist(gen) & 0xFFFF) << "-";
        ss << std::setw(4) << ((dist(gen) & 0x0FFF) | 0x4000) << "-";
        ss << std::setw(4) << ((dist(gen) & 0x3FFF) | 0x8000) << "-";
        ss << std::setw(12) << dist(gen) << dist(gen);

        return ss.str();
    }
}
