# NEXT.md

Handoff document for resuming work on **bit-backup**. Based only on the current
repo state (branch `develop`, HEAD `cec3346`) and observed build/test behavior.

---

## 1. Project summary

- **What:** `bit-backup` is a C++23 command-line tool that detects "bit rot"
  (silent data corruption) by storing SHA-512 checksums of files in a local
  SQLite database (`.bitbackup.sqlite3`) and re-verifying them on later runs.
- **Main goal:** long-term data integrity for large file trees.
- **Current phase:** the original transliterated-from-Java code has been made
  performant and extended with features. Core is stable; recent work focused on
  performance (batched DB writes, parallel hashing), `.bitbackupignore`
  correctness/speed, error handling, and a new directory-locking feature.
- **Important architectural decisions:**
  - Command pattern: `BitBackupProgram` dispatches to `CheckCommand` (default),
    `HelpCommand`, `VersionCommand`.
  - Persistence behind interfaces (`FileRepository`, `SystemItemRepository`) with
    a single SQLite implementation; one **shared** SQLite connection per run,
    all writes **batched in transactions**.
  - Schema evolves via an **append-only, hash-validated** migration array.
  - DB self-integrity hash uses a **rollback journal (NOT WAL)** so the single
    `.sqlite3` file is always complete after commit.
  - Hashing is parallel; **DB writes stay single-threaded**.

---

## 2. Current status

- **Build:** WORKS. CMake (Release) builds `bit_backup` and (with
  `-DENABLE_TESTS=ON`) the `Tests` target cleanly. Toolchain in use: GCC 14,
  CMake 3.31, OpenSSL 3.5, bundled SQLiteCpp + googletest submodules.
- **Tests:** PASS — `ctest` reports **46/46** passing.
- **CLI available:**
  - Commands: `check` (default when no command given), `help`, `version`.
  - `check` options: `dir=`, `report=true`, `verbose=true`, `bitbackupindex=true`,
    `threads=N`, `quick=true`, `scrub=N` (0–100).
  - Exit code: `check` returns **non-zero (1)** when bit rot OR a lock violation
    is found; `help`/`version` always return 0; unknown command/arguments print
    a clean error and return 1 (no more SIGABRT).
- **Recently implemented (working):** batched SQLite writes; parallel SHA-512
  hashing; `quick`/`scrub` modes; `.bitbackupignore` precompiled regex + fixed
  leading-slash/CRLF handling + directory pruning + negation/trailing-slash;
  graceful error handling; `.bitbackuplock` directory locking with a `LOCKED`
  DB column.
- **Working demos/examples:** the golden characterization scenario (add / modify
  / delete / silent bit rot) and the lock end-to-end tests all pass; manual
  verification of locking, quick/scrub, and the migration upgrade path was done.
- **What does NOT work yet / caveats:**
  - Running `check` with `dir=<path>` from a **different current working
    directory** is unsafe when bit rot is found (see §4).
  - Options must follow an explicit `check` (e.g. `bit_backup quick=true`
    alone is treated as an unknown command and errors out).
  - `quick` mode intentionally does not detect silent rot for non-locked files.

---

## 3. Recent changes (most recent first)

- `cec3346` Flag deleted locked files as `KO` in the DB (kept row, frozen
  mtime/hash, result set to KO).
- `4cd53a2` **Directory locking via `.bitbackuplock`**: migration #5 adds a
  `LOCKED` column; `FsFile.locked`; `CheckCommand` part4 lock-root detection +
  `isPathLocked`; part6/7/8 frozen-set semantics; red summary; non-zero exit;
  8 e2e lock tests + repo `LOCKED` round-trip test.
- `e3dc2f1` Catch exceptions in `main()` → clean red `Error:` + exit 1 instead
  of `std::terminate`/SIGABRT; `BitBackupProgram::run` now returns an exit code.
- `91052d5` End-to-end bit-rot detection tests (`CheckCommandBitRotTests.cpp`).
- `a3fcbc8` `.bitbackupignore` gitignore-style negation (`!`) + trailing-slash
  directory patterns.
- `ac328b5` Prune ignored directories during the scan (`disable_recursion_pending`).
- `e370e5b` Precompile ignore regexes once + fix leading-slash/CRLF + auto-ignore
  metadata files + unit tests.
- `bc9e2fa` Parallel hashing, `quick`/`scrub` modes, 1 MiB read buffer.
- `c457f6a` Batched SQLite writes (single connection, transactions); re-enabled
  the GTest target; fixed a CMake bug where `Main.cpp` leaked into the core lib.

Only untracked file: `.bitbackupignore` in the repo root (pre-existing, not part
of this work).

---

## 4. Current blocker / main problem

There is **no build or test blocker** — everything builds and `ctest` is green.

The most important **open correctness issue** is CWD-relative path handling in
the bit-rot summary and the CSV report:

- **Symptom:** when `check` is run with `dir=<somewhere-else>` from a different
  process working directory AND bit rot is found, the summary re-hash throws
  `File does not exist` (caught by `main` → prints `Error:` and exits 1) instead
  of printing the bit-rot report. With `report=true` the report rows are also
  computed against the wrong path.
- **Failing command (manual repro):** from `/tmp`, run
  `bit_backup check dir=/path/with/bitrot report=true` — the summary/report
  re-hash resolves `./<relativepath>` against `/tmp`, not against `dir=`.
- **Failing test:** none yet — the existing e2e tests `chdir` into the fixture
  dir, so they never exercise the `dir=` + foreign-CWD path. **Needs a test.**
- **Affected files/modules:** `src/BitBackup/Commands/CheckCommand.cpp`
  — the summary loop inside `run()` and `part9CreateReportCsvIfNeeded`, both of
  which build `File("./" + f.absolutePath)`.
- **Suspected cause:** those two spots use `"./" + absolutePath` (process CWD)
  instead of `bitBackupContext.getWorkingDirectory() + "/" + absolutePath`
  (which part8's detection already uses correctly).
- **Already tried:** nothing fixed yet; identified by code inspection. part8's
  actual detection/DB update is correct; only the human-facing summary/report
  use the wrong base path.

---

## 5. Known bugs and limitations

- **CONFIRMED BUG:** bit-rot summary + `part9` report use `"./" + absolutePath`
  (CWD-relative) — wrong/throws when `dir=` differs from the process CWD. See §4.
- **CONFIRMED (UX wart):** the first CLI argument is always the command, so
  options without `check` (`bit_backup quick=true`) error with
  "Invalid command!". Documented in README; not yet softened.
- **INCOMPLETE:** nested `.bitbackupignore` files are not loaded; only the root
  one is applied (the older recursive loader is dead code).
- **INCOMPLETE / DOCUMENTED:** `**` globstar is not special-cased — a single `*`
  already crosses `/` in this implementation, so patterns with a slash are
  broader than gitignore's single-level `*`.
- **BY DESIGN:** `quick`/partial `scrub` skip silent-rot detection for
  unchanged-modtime, non-locked files.
- **TECH DEBT:** dead code remains (`foundFilesInCurrentDir`,
  `Utils::listAllFilesInDir`); `part8` keeps an unused `filesToBeRemovedFromDb`
  parameter; `found.reserve(200000)` is a magic number; `BitBackupContext` uses
  raw `new`/`delete`. (A broad "step 6" cleanup was explicitly deferred by the
  owner — do not start it unprompted.)
- **SECURITY / NEEDS ACTION (owner):** `.git/config` `origin` URL contains a
  plaintext GitHub PAT. Recommend rotating it and using a credential helper / SSH.
- **MINOR:** `SqliteDatabaseMigration::getInstance()` allocates a singleton that
  is never freed; `getCurrentDateTime()` in the migration uses `std::localtime`
  (single-threaded there, so fine).

---

## 6. Architecture notes

- **Entry / dispatch:** `src/.../Core/Main.cpp` → `BitBackupProgram::run` →
  resolves the command by `getName()` → `Command::run(args)`. `main` catches
  `std::exception` and maps a non-empty `check` result to exit code 1.
- **Check flow (`CheckCommand::run`, parts 1–10):**
  1 verify DB self-hash · 2 migrate schema · 3 update version · 4 scan filesystem
  (collects ignore + lock roots) · 5 load DB rows · 6 add new files (parallel
  hash) · 7 remove deleted (lock-aware) · 8 compare content/modtime (parallel
  hash, lock-aware) · 9 optional CSV report · 10 recompute DB self-hash.
- **Key modules:** `Core/BitBackupIgnoreRegex` (precompiled patterns,
  `test()` / `matchesDirectoryContents()`), `Core/ListSet` (vector + hash-set),
  `Persistence/Impl/Sqlite/FileRepositoryImplSqlite` (shared connection, batched
  `create/list/updateAll/removeAll/updateLastCheckDate`),
  `Persistence/Impl/Sqlite/SqliteDatabaseMigration` (+ `Migrations.h`).
- **Data flow for locking:** part4 records dirs containing `.bitbackuplock` as
  `lockRoots` (`""` = working-dir root); `isPathLocked(rel)` checks ancestors;
  the `LOCKED` column persists the state so deletions are caught even after the
  marker is gone.
- **Invariants that MUST hold:**
  - Default (unlocked, no-flag) `check` behavior must stay byte-identical — the
    golden scenario is the guard.
  - `migrations[]` is **append-only**; existing entries are **hash-validated** at
    runtime and must never be edited. Add new migrations + bump `MIGRATION_COUNT`.
  - Keep the **rollback journal** (do not switch SQLite to WAL) — the `.sqlite3`
    self-integrity hash relies on the single file being complete after commit.
  - Hashing may be parallel; **all DB writes must stay single-threaded.**
  - `last_modified_string`/`print_clock` use `localtime_r` (thread-safe) because
    they run inside worker threads — keep it that way.
  - Locked files: never overwrite stored `LAST_MODIFICATION_DATE`,
    `HASH_SUM_VALUE`, `SIZE`.
- **Compatibility:** existing 4-migration DBs auto-upgrade to migration 5
  (verified). `FileRepository` is the persistence boundary; changing its
  interface affects `CheckCommand` and tests.

---

## 7. Useful commands

```bash
# Configure (with tests) and build everything
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j"$(nproc)"

# Run the tool
./build/bit_backup                 # = check, current directory
./build/bit_backup check dir=/path/to/data
./build/bit_backup check threads=8 quick=true
./build/bit_backup version
./build/bit_backup help

# Run the whole test suite
cd build && ctest --output-on-failure

# Run a single test group
./build/Tests --gtest_filter='CheckCommandLockTest.*'
./build/Tests --gtest_filter='CheckCommandBitRotTest.*'

# Reproduce the §4 bug (run from a DIFFERENT cwd than the data dir):
#   1) make a dir D with a file, run check inside D to index it
#   2) corrupt the file's bytes but restore its old mtime (silent rot)
#   3) from /tmp:  /abs/path/build/bit_backup check dir=D report=true
#      -> expect: should report bit rot; actually errors on a wrong "./path"

# Lock demo (frozen directory):
#   index a dir, then `touch <dir>/subdir/.bitbackuplock`, run check again,
#   then modify a file under subdir and run check -> violation + exit 1,
#   stored mtime/hash unchanged.
```

No linter/formatter is configured in the repo.

---

## 8. Next smallest tasks (ordered)

1. **Add a failing test for the `dir=` + foreign-CWD bit-rot path.**
   - Goal: lock in the §4 bug with a red test before fixing.
   - Files: new `tests/BitBackup/Commands/CheckCommandDirArgTests.cpp` (do NOT
     `chdir`; pass `dir=<temp>` while CWD stays elsewhere; cause silent rot).
   - Verify: `cd build && ctest` — the new test should FAIL initially.

2. **Fix CWD-relative paths in the summary + report.**
   - Goal: use the working directory instead of `"./"` so `dir=` works from any CWD.
   - Files: `src/BitBackup/Commands/CheckCommand.cpp` — the bit-rot summary loop
     in `run()` and `part9CreateReportCsvIfNeeded` (replace `"./" + f.absolutePath`
     with `bitBackupContext.getWorkingDirectory()` / `bitBackupFiles.workingDir`
     joined paths).
   - Verify: the task-1 test now PASSES; `ctest` stays 46+/all green; golden
     scenario still identical.

3. **Remove the unused `filesToBeRemovedFromDb` parameter from part8.**
   - Goal: kill a dead parameter and its warning.
   - Files: `CheckCommand.cpp` / `CheckCommand.h` (signature + the single caller
     in `run()`).
   - Verify: `cmake --build build` clean; `ctest` green.

4. **Make options-without-`check` not error (small UX fix) — OPTIONAL.**
   - Goal: `bit_backup quick=true` should behave like `check quick=true`
     (or at least exit cleanly). Decide semantics first.
   - Files: `Core/BitBackupArgs.cpp` (command detection) and/or
     `Core/BitBackupProgram.cpp`.
   - Verify: `./build/bit_backup quick=true; echo $?` returns 0 and runs a check;
     add a small test for the chosen behavior.

---

## 9. Do not do yet

- **No broad refactor / "step 6" cleanup** (DI, dead-code removal, smart
  pointers, OpenSSL EVP migration) — the owner explicitly deferred this.
- **Do not switch SQLite to WAL** — it breaks the `.sqlite3` self-integrity hash.
- **Do not edit existing `migrations[]` strings** — they are hash-validated;
  only append a new migration and bump `MIGRATION_COUNT`.
- **No `FileRepository` interface changes** without updating all callers and
  tests and checking the migration/DB compatibility path.
- **Do not change the default (unlocked, no-flag) `check` behavior** without
  re-running the golden scenario; it must stay identical.
- **No new subcommands** (e.g. `lock`/`unlock`/`status`) until §4 is fixed.
- **Do not "fix" the embedded git PAT in code** — that's an owner/ops action
  (rotate + credential helper), not a source change.

---

## 10. Resume prompt (copy-paste for a future Claude Code session)

```
Read NEXT.md in the repo root first. Work only on "Next smallest task #1"
(and then #2) from it: add a failing test that runs `check` with dir=<temp>
from a different current working directory and triggers silent bit rot, then
fix the CWD-relative "./"+absolutePath usage in CheckCommand.cpp's run()
summary and part9CreateReportCsvIfNeeded so it uses the working directory.

Inspect only the files needed for that task (CheckCommand.cpp/.h and the
existing CheckCommandBitRotTests.cpp as a template). Do not refactor unrelated
code, do not change the migrations array, do not switch to WAL, and do not
alter default unlocked check behavior. Make one small, verified change.

Build and test with:
  cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
  cmake --build build -j"$(nproc)"
  cd build && ctest --output-on-failure

Confirm the new test goes red-then-green and that the golden default behavior
is unchanged. Then update NEXT.md (move the finished task out, refresh status).
```
