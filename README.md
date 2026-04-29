
# Project Overview and Architecture Summary

**vcf_importer** reads a VCF file and stores its contents in an SQLite3 database.

- One thread opens the file and reads lines in batches.
- Each batch is submitted to a thread pool, where worker threads pick up the work.
- Each worker parses the lines in its batch and passes the parsed data to a storage thread for sequential database insertions in a `staging` table.
- Once all insertions into the staging table are complete, the main thread runs an optimized query to insert the data into the `variants` table in order.

# Build instructions

`mkdir build`<br>
`cd build` <br>
`cmake ..` <br>
`cmake --build .` <br>

# Required environment variables for database access
## Linux
`export SQLITEDBNAME=vcf_database.db`

## Visual Studio
- Right-click your project in Solution Explorer and select Properties.
- Go to Configuration Properties → Debugging.
- In the Environment field, add your variable like this
`SQLITEDBNAME=vcf_database.db`


# How to run the application (with examples)


`build/vcf_importer --vcf PATH --threads N`

## Command-Line Options

| Option              | Description                                      | Example                |
|---------------------|--------------------------------------------------|------------------------|
| `--vcf PATH`        | Path to the input VCF file                       | `--vcf data/input.vcf` |
| `--threads N`       | Number of threads to use for processing          | `--threads 4`          |
| `--log-level LEVEL` | Logging level: `INFO`, `WARNING`, `DEBUG`, `ERROR`. Default is `INFO` | `--log-level DEBUG`    |
| `--batch BATCH`           | Number of lines in each batch to be processed       | `--reset`              |
| `--reset`           | Clears the tables in storage before processing       | `--reset`              |
| `--help`, `-h`      | Show help message and exit                       | `--help`               |

# Filesystem

- apps: the main entry points files are here <br>
- src: the source files <br>
- include: the headers <br>
- Common: Library containing files needed accross the project <br>
- DataAccessLayer: Library for persisting and retrieving data<br>

# Third-Party Libraries

- gtest : automatically installed through cmake <br>
- nlohmann : automatically installed through cmake/nuget</br>

# DataAccessLayer

The `DataAccessLayer` is a separate shared library (DLL) that provides SQLite-backed persistence for VCF variants.

## Components

| Class / Struct | File | Responsibility |
|---|---|---|
| `IVCFDal` | `IVCFDal.h` | Abstract interface — defines the DAL contract |
| `SQLiteVCFDal` | `SQLiteVCFDal.h/.cpp` | Concrete SQLite implementation of `IVCFDal` |
| `SQLiteConnectionPool` | `SQLiteConnectionPool.h/.cpp` | Thread-safe pool of SQLite connections |
| `SQLiteQuery` | `SQLiteQuery.h/.cpp` | RAII wrapper for non-SELECT statements (INSERT, UPDATE, DELETE) |
| `SQLiteSelect` | `SQLiteSelect.h/.cpp` | Helper for stepping through SELECT result rows |
| `SQLiteStmt` | `SQLiteStmt.h/.cpp` | Low-level prepared-statement utility |
| `VCFSQLiteRecord` | `VCFSQLiteRecord.h/.cpp` | Internal record that maps between `VCFData` and a SQLite row |

## Public API (`IVCFDal`)

| Method | Description |
|---|---|
| `storeBatchInStaging(const vector<VCFData>&)` | Bulk-inserts a batch of variants into a staging table. |
| `finalizeRecords()` | Moves records from staging into the main table, sorted by chromosome and position. |
| `fetchAllVariants()` | Returns all stored variants sorted by chromosome and position. |

All methods throw `std::runtime_error` on SQLite failure.

## Data Storage Design

- Variants are first written to a **staging table** via `storeBatchInStaging()` for fast bulk insertion.
- `finalizeRecords()` merges staging into the **main table** in sorted order (chrom, pos).
- Each row stores `chromosome`, `position`, `ref`, and `alt` as dedicated columns. The remaining fields (`filter`, `qual`, `info`, `format`) are serialized as a **JSON blob** in a single `data` column by `VCFSQLiteRecord`.

## Thread Safety

`SQLiteConnectionPool` manages a pool of connections (default size: `10`). `acquire()` blocks until a connection is available; the caller must call `release()` when done. `SQLiteVCFDal` uses a scoped RAII helper (`ScopedDb`) to ensure connections are always released.

# Common

The `Common` library contains shared infrastructure used across the application, including configuration access, error handling, logging, concurrency primitives, and the core `VCFData` model. It exists to keep cross-cutting utilities in one reusable module so the application and other libraries can depend on a single common foundation instead of duplicating support code.

## Components

| Class / Utility | File | Responsibility |
|---|---|---|
| `Configuration` | `Configuration.h/.cpp` | Singleton configuration provider that reads required runtime settings such as the SQLite database name from environment variables. |
| `getEnvVar` | `Configuration.h/.cpp` | Helper for retrieving environment variable values used by configuration code. |
| `ErrorCodes` | `ErrorHandler.h` | Shared enum describing application error and warning categories. |
| `RaiseError` / `RaiseWarning` | `ErrorHandler.h/.cpp` | Centralized helpers for surfacing fatal errors and warnings consistently across modules. |
| `Logger` | `Logging.h` | Process-wide logger with log levels, timestamps, source location output, and synchronized writes. |
| `LOG` | `Logging.h` | Convenience macro that captures file and line information automatically when logging. |
| `ThreadPool` | `ThreadPool.h/.cpp` | worker pool for dispatching background tasks and waiting for all queued work to finish. |
| `VCFData` | `VCFData.h/.cpp` | Shared DTO representing a parsed VCF record, including typed INFO and FORMAT maps. |
| `CommonExport.h` | `CommonExport.h` | Export/import macro definitions for building and consuming the Common library on Windows. |

## Design And Conventions

- The library centralizes reusable, low-level services that should not depend on higher-level parsing or persistence modules.
- `Configuration` is implemented as a non-copyable singleton so shared runtime settings are initialized once and reused consistently.
- `ThreadPool` encapsulates worker lifecycle, task queue synchronization, and completion waiting behind a small API.
- `Logger` uses a mutex to serialize output so log lines are not interleaved across threads.
- `VCFData` uses `std::variant` and `std::unordered_map` so parsed INFO and FORMAT values can preserve basic type information without a rigid schema.
- DLL visibility is controlled through `COMMON_API`, which keeps the library consumable from other projects on MSVC while remaining portable on non-Windows builds.

## How To Use

- Link other modules against the `Common` target or the generated `Common` library artifact.
- Include headers directly from the `Common` directory, for example `#include "Logging.h"`, `#include "ThreadPool.h"`, or `#include "VCFData.h"`.
- Use `LOG(level, message)` instead of writing directly to `std::cout` when you want consistent timestamps and file/line metadata.
- Use `Configuration::getInstance()` for runtime settings instead of reading environment variables ad hoc throughout the codebase.
- Use `ThreadPool` for coarse-grained background work and call `waitForTasksToFinish()` before finalization or shutdown paths that depend on queued work being complete.

## Dependencies And Requirements

- Standard C++ library facilities only: strings, streams, threads, mutexes, condition variables, variants, and unordered maps.
- Runtime configuration expects the `SQLITEDBNAME` environment variable to be set before the application starts.
- On Windows, consumers must compile with the correct import/export settings so `COMMON_API` resolves to `__declspec(dllexport)` or `__declspec(dllimport)` as intended.
- The logging utility currently writes to standard output; consumers should treat it as a process-wide shared service.

# Validators

The `Validators` module defines the reusable validation rules applied to parsed VCF tokens before they are accepted by the parser or passed deeper into the pipeline. It provides a small set of composable validators and a default mapping from VCF token positions to validation chains, so parsing logic can stay focused on field extraction instead of hard-coding validation rules inline.

## Components

| Class / Utility | File | Responsibility |
|---|---|---|
| `Validator` | `include/Validators.h`, `src/Validators.cpp` | Base validation type that supports chaining another validator and provides the common `validate()` interface. |
| `HasValue` | `include/Validators.h`, `src/Validators.cpp` | Rejects empty tokens. |
| `NoCharAllowed` | `include/Validators.h`, `src/Validators.cpp` | Rejects tokens containing a specified forbidden character. |
| `NoWhiteSpace` | `include/Validators.h`, `src/Validators.cpp` | Specialization of `NoCharAllowed` that rejects spaces inside a token. |
| `NoSemicolon` | `include/Validators.h`, `src/Validators.cpp` | Specialization of `NoCharAllowed` that rejects semicolons. |
| `NoZero` | `include/Validators.h`, `src/Validators.cpp` | Specialization of `NoCharAllowed` that rejects the character `0`. |
| `MatchRegex` | `include/Validators.h`, `src/Validators.cpp` | Validates a token against a regular expression. |
| `NoDuplicates` | `include/Validators.h`, `src/Validators.cpp` | Tracks previously seen values and rejects duplicates. |
| `Required` | `include/Validators.h`, `src/Validators.cpp` | Rejects the VCF missing-value marker `.` for fields that must be present. |
| `IsInteger` | `include/Validators.h`, `src/Validators.cpp` | Ensures a token parses completely as an integer. |
| `IsFloat` | `include/Validators.h`, `src/Validators.cpp` | Ensures a token parses completely as a floating-point number. |
| `AllowValue` | `include/Validators.h`, `src/Validators.cpp` | Top-level bypass validator that accepts one specific value and skips the rest of the chain. |
| `AllowMissing` | `include/Validators.h`, `src/Validators.cpp` | Specialization of `AllowValue` that accepts the missing marker `.`. |
| `AllowPass` | `include/Validators.h`, `src/Validators.cpp` | Specialization of `AllowValue` that accepts `PASS`. |
| `idsForValidation` | `include/Validators.h` | Declares which token positions are expected to have validation applied. |
| `ValidationsType` | `include/Validators.h` | Alias for the map that binds token ids to validator chains. |
| `DefaultValidations` | `include/Validators.h` | Prebuilt validator map for core VCF fields such as chromosome, position, id, ref, alt, and qual. |

## Design And Conventions

- The module uses the Decorator pattern: each validator can wrap another validator, allowing small single-purpose checks to be combined into a validation pipeline.
- Validators throw `std::runtime_error` on failure, which keeps calling code simple and lets parsing code decide whether to reject, warn, or recover from a bad token.
- `AllowValue`-style validators are intended to sit at the top of a chain because they short-circuit further validation when the allowed token is encountered.
- Stateful rules such as `NoDuplicates` retain seen values across validations, so the owning parser should control validator lifetime carefully when duplicate tracking scope matters.
- `DefaultValidations` centralizes the project’s default field rules so new parser instances can start from a consistent validation baseline.

## How To Use

- Include `Validators.h` where token validation is part of parsing or field normalization.
- Build custom validation chains by nesting validators from the innermost rule outward, then call `validate(token)` on the top-level validator.
- Reuse `DefaultValidations` when standard VCF field validation is sufficient, or provide your own `ValidationsType` map when field requirements differ.
- Keep bypass validators like `AllowMissing` or `AllowPass` at the top of the chain so sentinel values are accepted before stricter checks run.
- Share validator instances only when shared state is intentional; create fresh chains when you need isolated state for duplicate tracking or parser-local behavior.

## Dependencies And Requirements

- Depends on `VCFConstants.h` for token identifiers such as `TOKEN_CHROM`, `TOKEN_POS`, and related field constants.
- Uses only standard C++ library facilities beyond that: smart pointers, unordered containers, strings, and regular expressions.
- Consumers should expect validation failures to surface as exceptions and handle them at parser or orchestration boundaries.
- The default validator map currently covers the main fixed VCF columns `CHROM`, `POS`, `ID`, `REF`, `ALT`, and `QUAL`; INFO and FORMAT validation remain separate concerns.


# Example Running

## Example running with log-level INFO

Example running `./vcf_importer --vcf ../tests/assignment.sample.vcf --threads 10 --log-level INFO --reset`

<details>
<summary>Show output</summary>

```sh
henra@DESKTOP-8HJU1KE:/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/build$ ./vcf_importer --vcf ../tests/assignment.sample.vcf --threads 10 --reset
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/DataAccessLayer/SQLiteVCFDal.cpp:27] Creating tables if they do not exist...
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/DataAccessLayer/SQLiteVCFDal.cpp:169] Clearing existing data from database...
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:86] Starting VCF processing with file: ../tests/assignment.sample.vcf using 10 threads
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:19] Starting to read file and dispatch batches to workers...
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:31] Main thread finished reading file and dispatching batches. Waiting for workers to finish...
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:43] Thread 123336302524096 starting batch parsing
2026-04-29 10:03:26 [WARNING] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:54] Error parsing line: chr6       148275936 .       T       C       166.77  PASS    AC=1;AF=0.5;AN=2;BaseQRankSum=-0.629;ClippingRankSum=.;DP=14;ExcessHet=3.0103;FS=0;MLEAC=1;MLEAF=0.5;MQ=59.86;MQRankSum=-0.674;QD=11.91;ReadPosRankSum=-1.325;SOR=0.79    GT:AB:AD:DP:GQ:PL:SAC   0/1:0.5:7,7:14:99:195,0,156:4,3,3,4. Error: stof
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:38] All workers finished. Finalizing records...
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:94] Fetching all records from DB to verify insertion
2026-04-29 10:03:26 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:98] Total records in storage : 22
```
</details>

## Example running with log-level DEBUG

Example running `./vcf_importer --vcf ../tests/assignment.sample.vcf --threads 10 --log-level DEBUG --reset`

<details>
<summary>Show output</summary>

```sh

henra@DESKTOP-8HJU1KE:/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/build$ ./vcf_importer --vcf ../tests/assignment.sample.vcf --threads 10 --log-level DEBUG --reset
2026-04-29 10:09:30 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/DataAccessLayer/SQLiteVCFDal.cpp:27] Creating tables if they do not exist...
2026-04-29 10:09:30 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/DataAccessLayer/SQLiteVCFDal.cpp:169] Clearing existing data from database...
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:86] Starting VCF processing with file: ../tests/assignment.sample.vcf using 10 threads
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:19] Starting to read file and dispatch batches to workers...
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:31] Main thread finished reading file and dispatching batches. Waiting for workers to finish...
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:43] Thread 139822893102784 starting batch parsing
2026-04-29 10:09:31 [WARNING] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:54] Error parsing line: chr6       148275936 .       T       C       166.77  PASS    AC=1;AF=0.5;AN=2;BaseQRankSum=-0.629;ClippingRankSum=.;DP=14;ExcessHet=3.0103;FS=0;MLEAC=1;MLEAF=0.5;MQ=59.86;MQRankSum=-0.674;QD=11.91;ReadPosRankSum=-1.325;SOR=0.79    GT:AB:AD:DP:GQ:PL:SAC   0/1:0.5:7,7:14:99:195,0,156:4,3,3,4. Error: stof
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/src/VCFProcessor.cpp:38] All workers finished. Finalizing records...
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:94] Fetching all records from DB to verify insertion
2026-04-29 10:09:31 [INFO] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:98] Total records in storage : 22
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr10, pos: 49825338, ref: T, alt: A, filter: PASS, qual: 485.77, info: {SOR: 0.825, ReadPosRankSum: 0.896, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 0.568, AC: 1, MQ: 60, DP: 41, ExcessHet: 3.0103, FS: 6.658, MLEAC: 1, MQRankSum: 0, QD: 11.85}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr11, pos: 87278074, ref: T, alt: C, filter: PASS, qual: 235.77, info: {SOR: 1.402, ReadPosRankSum: 0.141, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: -2.283, AC: 1, MQ: 60, DP: 27, ExcessHet: 3.0103, FS: 1.657, MLEAC: 1, MQRankSum: 0, QD: 8.73}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr12, pos: 5317850, ref: T, alt: C, filter: PASS, qual: 1100.77, info: {SOR: 1.493, QD: 28.97, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 38, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr13, pos: 60567622, ref: C, alt: T, filter: PASS, qual: 386.77, info: {SOR: 1.286, ReadPosRankSum: 0.977, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 1.454, AC: 1, MQ: 60, DP: 31, ExcessHet: 3.0103, FS: 8.272, MLEAC: 1, MQRankSum: 0, QD: 12.48}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr16, pos: 73428910, ref: A, alt: G, filter: PASS, qual: 345.77, info: {SOR: 0.645, ReadPosRankSum: -0.351, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 0.568, AC: 1, MQ: 60, DP: 31, ExcessHet: 3.0103, FS: 1.4, MLEAC: 1, MQRankSum: 0, QD: 11.15}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr16, pos: 83514087, ref: G, alt: C, filter: PASS, qual: 1136.77, info: {SOR: 1.206, QD: 27.73, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 42, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr17, pos: 65333176, ref: G, alt: A, filter: PASS, qual: 683.77, info: {SOR: 0.874, QD: 31.08, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 22, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr19, pos: 11294537, ref: A, alt: C, filter: PASS, qual: 454.77, info: {SOR: 2.024, ReadPosRankSum: -0.752, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 0.314, AC: 1, MQ: 59.77, DP: 33, ExcessHet: 3.0103, FS: 8.039, MLEAC: 1, MQRankSum: 1.031, QD: 13.78}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr19, pos: 36540583, ref: A, alt: AT, filter: PASS, qual: 1768.73, info: {SOR: 0.952, QD: 35.37, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 51, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr2, pos: 128793684, ref: T, alt: TTT, filter: PASS, qual: 212.73, info: {SOR: 4.804, QD: 21.27, MLEAC: 1, FS: 0, ExcessHet: 3.0103, MQ: 58.44, DP: 16, AN: 2, MLEAF: 0.5, AF: 0.5, AC: 1}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr2, pos: 175395461, ref: A, alt: AT, filter: PASS, qual: 272.73, info: {SOR: 1.815, ReadPosRankSum: -2.137, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 0.806, AC: 1, MQ: 60, DP: 43, ExcessHet: 3.0103, FS: 13.426, MLEAC: 1, MQRankSum: 0, QD: 7.37}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr3, pos: 109754177, ref: T, alt: C, filter: PASS, qual: 937.77, info: {SOR: 0.756, QD: 30.25, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 31, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr4, pos: 8586596, ref: G, alt: A, filter: PASS, qual: 295.77, info: {SOR: 0.361, ReadPosRankSum: 0.276, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 1.303, AC: 1, MQ: 60, DP: 33, ExcessHet: 3.0103, FS: 1.574, MLEAC: 1, MQRankSum: 0, QD: 8.96}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr4, pos: 120022633, ref: GA, alt: G, filter: PASS, qual: 468.73, info: {SOR: 0.307, ReadPosRankSum: -0.109, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: -0.768, AC: 1, MQ: 58.98, DP: 25, ExcessHet: 3.0103, FS: 1.721, MLEAC: 1, MQRankSum: 0.058, QD: 18.75}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr6, pos: 31077600, ref: G, alt: T, filter: PASS, qual: 630.77, info: {SOR: 1.22, ReadPosRankSum: -0.502, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 4.149, AC: 1, MQ: 60, DP: 44, ExcessHet: 3.0103, FS: 2.661, MLEAC: 1, MQRankSum: 0, QD: 14.34}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr6, pos: 32687689, ref: G, alt: A, filter: PASS, qual: 1759.77, info: {SOR: 1.788, QD: 43.99, MLEAC: 2, FS: 0, ExcessHet: 3.0103, MQ: 60, DP: 43, AN: 2, MLEAF: 1, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr6, pos: 167911080, ref: A, alt: G, filter: PASS, qual: 234.77, info: {SOR: 0.626, ReadPosRankSum: 0.3, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: -3.759, AC: 1, MQ: 60, DP: 29, ExcessHet: 3.0103, FS: 1.455, MLEAC: 1, MQRankSum: 0, QD: 8.1}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr8, pos: 7368927, ref: C, alt: CAA, filter: PASS, qual: 572.73, info: {SOR: 3.611, QD: 52.07, MLEAC: 1, FS: 0, ExcessHet: 3.0103, MQ: 32.26, DP: 20, AN: 2, MLEAF: 0.5, AF: 1, AC: 2}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr9, pos: 4274786, ref: C, alt: T, filter: PASS, qual: 368.77, info: {SOR: 0.374, ReadPosRankSum: 0.116, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 1.923, AC: 1, MQ: 60, DP: 34, ExcessHet: 3.0103, FS: 1.394, MLEAC: 1, MQRankSum: 0, QD: 10.85}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr9, pos: 66769666, ref: C, alt: T, filter: FAIL, qual: 40.77, info: {SOR: 1.175, ReadPosRankSum: 0.808, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: 3.641, AC: 1, MQ: 47.77, DP: 57, ExcessHet: 3.0103, FS: 4.144, MLEAC: 1, MQRankSum: -3.447, QD: 0.73}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chr9, pos: 97682075, ref: A, alt: T, filter: PASS, qual: 251.77, info: {SOR: 1.204, ReadPosRankSum: 0.604, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: -0.83, AC: 1, MQ: 60, DP: 27, ExcessHet: 3.0103, FS: 1.792, MLEAC: 1, MQRankSum: 0, QD: 9.32}, format: {}
2026-04-29 10:09:31 [DEBUG] [/mnt/c/Users/henra/Documents/workspace/C++/VisualStudio/Saphetor/apps/main.cpp:101] chrom: chrX, pos: 61963914, ref: CA, alt: C, filter: PASS, qual: 370.73, info: {SOR: 0.776, ReadPosRankSum: -0.72, ClippingRankSum: 0, AF: 0.5, MLEAF: 0.5, AN: 2, BaseQRankSum: -1.026, AC: 1, MQ: 60, DP: 32, ExcessHet: 3.0103, FS: 0, MLEAC: 1, MQRankSum: 0, QD: 11.59}, format: {}
```

</details>

# Not implemented

- Reading FORMAT fields
- Validating INFO values