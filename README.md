# kvstore-ycsb-perf

A small **C++17 key-value store** built as a mini storage engine project.

This project is designed to demonstrate **systems fundamentals**: correctness, durability, crash recovery, indexing, compaction, and concurrency safety (with tests), as preparation for system/software engineering internship interviews.

---

## Features

- **PUT / GET / DEL** operations via a simple CLI
- **Durability** via an **append-only log** (AOF-style)
- **Crash-safe recovery** by replaying the log on startup  
  - safely stops replay if the final record is truncated/corrupt
- **On-disk indexing**: `key -> (byte offset, size)`  
  - values are read directly from the log using seek + read
- **Compaction**: rewrites the log to keep only the latest live values (shrinks file, speeds recovery)
- **Thread safety** using a reader-writer lock (`std::shared_mutex`)
  - shared lock for GET, exclusive lock for PUT/DEL/Compact/Replay
- **Unit tests** (GoogleTest), including recovery + concurrency tests

---

## Project Structure

```text
include/kvstore/     Public headers
src/                Implementation + CLI
tests/              GoogleTest unit tests
docs/               Architecture/benchmark notes
.github/workflows/   CI (GitHub Actions)
