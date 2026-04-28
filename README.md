
# Project overview and architecture summary

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
| `--log-level LEVEL` | Logging level: `INFO`, `WARNING`, `DEBUG`, `ERROR` | `--log-level DEBUG`    |
| `--reset`           | Clears the tables in storage before import       | `--reset`              |
| `--help`, `-h`      | Show help message and exit                       | `--help`               |

# Filesystem

- src: the source files <br>
- include: the headers <br>

# Third-Party Libraries

gtest : automatically installed through cmake <br>
nlohmann : automatically installed through cmake/nuget

# Logging

Logs are written in `app.log` file

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
| `initDb(bool clearDb = false)` | Opens/creates the database and sets up tables. Pass `true` to wipe existing data. |
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


# Not implemented

- Reading FORMAT fields
- Validating INFO values