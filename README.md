# bit-backup

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)

**bit-backup** is a command-line utility designed for long-term data integrity. It prevents "bit rot" (silent data corruption) by storing and verifying SHA-512 checksums of your files in a local SQLite database.

## Key Features
- **Integrity Checks**: Detects changes in files by comparing current SHA-512 hashes with stored ones.
- **Ignore Patterns**: Exclude specific files or directories using `.bitbackupignore` (wildcards supported).
- **SQLite Backend**: Efficiently stores file metadata and hashes.
- **Self-Integrity**: Verifies the integrity of the backup database itself using a separate hash sum.
- **Reporting**: Generates reports for detected bit rot and full file indexes in CSV format.

## Getting Started

### Prerequisites
- **C++23** compliant compiler (GCC 12+, Clang 16+, or MSVC 2022)
- **CMake** 3.25+
- **OpenSSL** library (for cryptographic hashing)
- **SQLite3** (bundled as a submodule)

### How to Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Usage

The primary command is `check`, which scans the specified directory and updates the integrity database.

```bash
# Basic check (current directory)
./build/bit_backup

# Explicit check on a specific directory
./build/bit_backup check dir=/path/to/my/data

# Generate a bit rot report
./build/bit_backup check report=true

# Enable verbose logging
./build/bit_backup check verbose=true
```

### Commands
- `check`: (Default) Scans files and verifies hashes.
- `help`: Shows help information.
- `version`: Shows the tool version.

### Arguments for `check`
| Argument | Description | Default |
|----------|-------------|---------|
| `dir`    | Path to the directory to be checked for bit rot. | `.` (Current) |
| `report` | Set to `true` to generate a report file (`.bitbackupreport.csv`). | `false` |
| `verbose`| Set to `true` to show detailed scan information. | `false` |
| `bitbackupindex` | Set to `true` to generate a full file index (`.bitbackupindex.csv`). | `false` |
| `threads` | Number of worker threads used for hashing. | CPU cores |
| `quick`  | `true` skips re-hashing files whose modification time is unchanged. Fast, but does **not** detect silent bit rot. | `false` |
| `scrub`  | Re-hash only the oldest `N`% of unchanged-modtime files this run (rotating coverage, like a scrub). `100` = full check, `0` = same as `quick`. | `100` |

> Note: options must follow the explicit `check` command, e.g. `bit_backup check quick=true threads=8`.

### Performance
Hashing runs in parallel across `threads` workers, and all database
inserts/updates/deletes are batched into single transactions. For routine
runs over very large trees, `quick=true` (skip unchanged files) or
`scrub=N` (verify a rotating slice each run) keep wall-clock bounded while
`scrub` still eventually re-verifies everything.

### Excluding Files (`.bitbackupignore`)
Create a file named `.bitbackupignore` in the root of your scanned directory. You can use wildcards:
```ignore
# Ignore specific folders
/logs/*
/tmp/*

# Ignore specific file types
*.tmp
*.log
```

### Metadata Files
The tool creates several hidden files in the target directory to manage its state:
- `.bitbackup.sqlite3`: The database containing file metadata and hashes.
- `.bitbackup.sqlite3.sha512`: A hash of the database to ensure its own integrity.
- `.bitbackupreport.csv`: Generated when `report=true`, listing files with detected corruption.
- `.bitbackupindex.csv`: Generated when `bitbackupindex=true`, containing a list of all scanned files.

## Developer Information

### Tech Stack
- **C++23**: Utilizing modern features like `std::filesystem`.
- **SQLiteCpp**: A clean C++ wrapper for SQLite3.
- **OpenSSL**: For high-performance SHA-512 hashing.

### Project Structure
- `include/BitBackup/`: Public headers organized by module (Core, Commands, Entity, Files, Persistence).
- `src/BitBackup/`: Private implementation files.
- `tests/`: GTest-based unit tests for core functionality.
- `thirdparty/`: External dependencies (SQLiteCpp).

### Running Tests
(Note: Tests are optional and disabled by default)
```bash
# Enable tests during configuration
cmake -B build -DENABLE_TESTS=ON
cmake --build build

# Run tests
cd build
ctest
```

## Todo
- [ ] Table FILE – add new columns – linux_rights, owner, group.
- [ ] Improve performance for large datasets (>1M files).

---
License: MIT
