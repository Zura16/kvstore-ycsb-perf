# KVStore Architecture

## Goals
- Simple key-value store in C++
- Durable writes via append-only log (AOF-style)
- Crash-safe recovery by replaying log on startup

## Data model
- In-memory map: key -> value
- Persistence: append-only log file containing PUT/DEL operations

## Log format
PUT <key> <value_size>\n
<value_bytes>\n
DEL <key>\n

## Recovery
On startup, KVStore replays the log from the beginning:
- Applies PUT/DEL records to reconstruct the final state.
- If the final record is truncated/corrupt (e.g., crash mid-write), replay stops safely and keeps all earlier valid state.
