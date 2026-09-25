# NEXT.md

Handoff document for resuming work on **bit-backup**. Updated for the current
working tree and observed build/test behavior.

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
  - Hashing is storage-aware: HDD/unknown storage defaults to one worker,
    SATA/general SSD to at most four, and NVMe to at most 16;
    `threads=N` overrides this up to 16.
    **DB writes stay single-threaded**.

---

## 2. Current status

- **Build:** WORKS. CMake (Release) builds `bit_backup` and (with
  `-DENABLE_TESTS=ON`) the `Tests` target cleanly. Toolchain in use: GCC 14,
  CMake 3.31, OpenSSL 3.5, bundled SQLiteCpp + googletest submodules.
- **Tests:** PASS — `ctest` reports **70/70** passing.
- **CLI available:**
  - Commands: `check` (default when no command given), `help`, `version`.
  - `check` options: `dir=`, `report=true`, `verbose=true`, `bitbackupindex=true`,
    `threads=N`, `quick=true`, `scrub=N` (0–100), `confirm=delete` (interactive,
    per-violation permanent-removal prompt for stuck locked-file deletions).
  - Exit code: `check` returns **non-zero (1)** when bit rot OR a lock violation
    is found; `help`/`version` without options return 0; unknown command/arguments print
    a clean error and return 1 (no more SIGABRT).
- **Recently implemented (working):** CSV file index with relative path, byte
  size, and SHA-512 for every included file; correct bit-rot summary and CSV
  report paths when `dir=` points outside the process CWD; relative `dir=`
  scanning; batched SQLite writes; storage-aware parallel SHA-512 hashing;
  `quick`/`scrub` modes; `.bitbackupignore` precompiled regex + fixed
  leading-slash/CRLF handling + directory pruning + negation/trailing-slash;
  graceful error handling; `.bitbackuplock` directory locking with a `LOCKED`
  DB column.
- **Working demos/examples:** the golden characterization scenario (add / modify
  / delete / silent bit rot) and the lock end-to-end tests all pass; manual
  verification of locking, quick/scrub, and the migration upgrade path was done.
- **What does NOT work yet / caveats:**
  - Options must follow an explicit `check` (e.g. `bit_backup quick=true`
    alone is treated as an unknown command and errors out).
  - `quick` mode intentionally does not detect previously unknown silent rot for
    non-locked files; files already marked corrupt are still verified.

---

## 3. Recent changes (most recent first)

- **Working tree:** index output is written to a unique sibling file and renamed
  over `.bitbackupindex.csv`, so an existing symlink cannot overwrite its target.
  Temporary index files are excluded from later scans. The shared relative-path
  helper now handles `dir=` with a trailing slash; repeated runs keep the correct
  DB row and still detect silent corruption. CLI validation explicitly rejects
  `threads=200` as outside the supported range. Regression tests cover these
  edge cases.
- **Working tree:** partial `scrub` now rounds a positive percentage up to at
  least one file and advances `LAST_CHECK_DATE` only for files actually hashed,
  so repeated runs rotate through the collection. Already known corruption is
  still checked in quick mode. Check arguments reject unknown names and invalid
  values; `dir=` preserves embedded `=`. Metadata ignore rules cannot be
  overridden by `!` negation. Regression tests cover each case.
- **Working tree:** fixed `dir=` bit-rot summary/report path resolution and
  relative-path scanning, and implemented `bitbackupindex=true` in the active scanner. The index is a
  semicolon-delimited CSV with `path;size;sha512`, one included file per row.
  `CheckCommandCliOutputsTests.cpp` covers both regressions, including CSV
  quoting, a relative `dir=`, and a second index run that must not index its own output.
- `6eaae6b` **Feature:** `check confirm=delete` interactively asks, per
  "locked file deleted" violation, whether to permanently remove it from the
  DB (`y` removes it — even overriding an active lock — anything else/EOF
  leaves it exactly as before). This is the supported way to resolve
  violations that can never auto-clear (e.g. a whole locked subtree deleted
  together with its `.bitbackuplock` marker in one shot) without resorting to
  raw SQLite edits, which would desync the DB's own self-integrity checksum
  and make the next `check` abort at part 1. `CheckCommand` gained a second
  constructor taking an injectable `std::istream&` (defaults to `std::cin`)
  so the prompt is testable without real stdin. Added
  `CheckCommandLockTest.ConfirmDeleteAcceptsBypassDeletionWhenConfirmed` /
  `.ConfirmDeleteKeepsViolationWhenDeclined`. Only covers deletion-type
  violations so far — "locked file modified" / "new file in locked
  directory" are not yet confirmable this way.
- `2dc752e` **Fix:** removing `.bitbackuplock` after a file was deleted while
  locked now actually unlocks it. Previously `part7RemoveDeletedFilesFromDb`
  kept `fileInDb.locked == 1` as a permanent fallback, so a "locked file
  deleted" violation could never clear even after the marker was removed. Now
  it only stays a violation if the file's containing directory is *also* gone
  (the "whole locked subtree deleted at once" bypass this fallback exists to
  catch, per `DeletedLockedFileWithMarkerGoneStillReported`); if the directory
  still exists, removing the marker resumes normal deletion handling and the
  row is finally removed, matching the README's documented contract. Added
  `CheckCommandLockTest.UnlockingResolvesPreviouslyReportedDeletion`.
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

The working tree also contains the `web/` presentation; inspect `git status`
before resuming.

---

## 4. Resolved correctness issues

There is **no build or test blocker** — everything builds and `ctest` is green.

- **Foreign-CWD `dir=` bit rot:** the terminal summary and CSV report now join
  stored relative file paths to the scanned directory. The regression test
  stays in its original CWD, corrupts a file without changing its mtime, and
  checks both output hashes and the report contents. The scanner also computes
  relative file paths correctly when `dir=` itself is relative.
- **Missing `bitbackupindex=true` file:** the active scanner now writes
  `.bitbackupindex.csv` with deterministic file order, relative paths, sizes,
  and SHA-512 hashes. Ignored files and Bit Backup metadata stay out of it.
  Index generation reads every included file, even with `quick=true`.

---

## 5. Known bugs and limitations

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
  (collects ignore + lock roots and optionally writes the file index) · 5 load DB rows · 6 add new files (parallel
  hash) · 7 remove deleted (lock-aware) · 8 compare content/modtime (parallel
  hash, lock-aware) · 9 optional CSV report · 10 recompute DB self-hash.
- **Key modules:** `Core/BitBackupIgnoreRegex` (precompiled patterns,
  `test()` / `matchesDirectoryContents()`), `Core/ListSet` (vector + hash-set),
  `Persistence/Impl/Sqlite/FileRepositoryImplSqlite` (shared connection, batched
  `create/list/updateAll/removeAll/updateLastCheckDate`),
  `Persistence/Impl/Sqlite/SqliteDatabaseMigration` (+ `Migrations.h`).
- **Data flow for locking:** part4 records dirs containing `.bitbackuplock` as
  `lockRoots` (`""` = working-dir root); `isPathLocked(rel)` checks ancestors;
  the `LOCKED` column persists the state so a deletion is still caught as a
  violation if the marker and its directory vanish together in one shot. If
  only the marker is removed (the directory still exists), `part7` now treats
  the persisted `LOCKED` flag as resolved and lets the deletion resume normal
  handling (row removed) — see the `2dc752e` fix in §3. Violations that still
  can't auto-resolve (whole subtree + marker deleted together) require an
  explicit per-item `y` under `check confirm=delete` (`6eaae6b`) to remove.
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
./build/Tests --gtest_filter='CheckCommandCliOutputsTest.*'

# Foreign-CWD regression: use check dir=D report=true from another directory
# after silently corrupting a file in D. The terminal and CSV hashes must match.

# Lock demo (frozen directory):
#   index a dir, then `touch <dir>/subdir/.bitbackuplock`, run check again,
#   then modify a file under subdir and run check -> violation + exit 1,
#   stored mtime/hash unchanged.

# confirm=delete demo (resolving a stuck locked-deletion violation):
#   index a dir, lock it, delete the whole subtree + marker in one shot,
#   run check (violation, stuck forever without confirm), then:
#   ./build/bit_backup check confirm=delete
#   -> prompts "... Permanently remove from the database? [y/N]:" per item.
```

No linter/formatter is configured in the repo.

---

## 8. Next smallest tasks (ordered)

1. **Remove the unused `filesToBeRemovedFromDb` parameter from part8.**
   - Goal: kill a dead parameter and its warning.
   - Files: `CheckCommand.cpp` / `CheckCommand.h` (signature + the single caller
     in `run()`).
   - Verify: `cmake --build build` clean; `ctest` green.

2. **Make options-without-`check` not error (small UX fix) — OPTIONAL.**
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
- **No new subcommands** (e.g. `lock`/`unlock`/`status`) without a concrete
  request; `check confirm=delete` is a `check` option.
- **Do not "fix" the embedded git PAT in code** — that's an owner/ops action
  (rotate + credential helper), not a source change.

---

## 10. Resume prompt (copy-paste for a future Claude Code session)

```
Read NEXT.md in the repo root first. The foreign-CWD bit-rot and missing index
bugs have regression tests and are fixed. For the next task, remove the unused
`filesToBeRemovedFromDb` parameter from part8 without refactoring unrelated
code. Do not change the migrations array, switch to WAL, or alter default
unlocked check behavior.

Build and test with:
  cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
  cmake --build build -j"$(nproc)"
  cd build && ctest --output-on-failure

Confirm the full test suite and golden default behavior remain green. Then
update NEXT.md with the observed result.
```
