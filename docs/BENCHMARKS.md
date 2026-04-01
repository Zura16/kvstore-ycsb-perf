# Benchmarks

## Microbench (local)
Command examples:
- In-memory:
  - ./build-release/microbench --keys 200000 --ops 500000 --value_size 64 --read_ratio 0.8
- Persistent (AOF):
  - ./build-release/microbench --persistent --keys 50000 --ops 200000 --value_size 64 --read_ratio 0.8

Record results below.

### Environment
- Machine:
- OS:
- Compiler:
- Build type:

### Results
| Mode | keys | ops | value_size | read_ratio | throughput (ops/s) | p50 (us) | p95 (us) | p99 (us) |
|------|------|-----|------------|------------|--------------------|----------|----------|----------|
| in-memory |  |  |  |  |  |  |  |  |
| persistent |  |  |  |  |  |  |  |  |

## HTTP Load Test (hey)

Environment:
- Machine: (fill)
- OS: (fill)
- Build: (Debug or Release)  <-- important
- Server: kv_http_server (cpp-httplib)

### PUT (20k req, concurrency 50)
Command:
- hey -n 20000 -c 50 -m POST -d "1234567890" "http://127.0.0.1:8080/put?key=a"

Results:
- RPS: 40760.84
- p50: 0.4 ms, p95: 2.2 ms, p99: 9.4 ms
- slowest: 97.7 ms
- status: 200/20000

### GET (20k req, concurrency 50)
Command:
- hey -n 20000 -c 50 "http://127.0.0.1:8080/get?key=a"

Results:
- RPS: 54054.74
- p50: 0.4 ms, p95: 1.4 ms, p99: 6.3 ms
- slowest: 67.5 ms
- status: 200/20000

## HTTP Benchmarks

### hey (single hot key, localhost, concurrency=50, n=20000)

| Endpoint | Method | Payload | RPS | p50 | p95 | p99 | Status |
|---|---:|---:|---:|---:|---:|---:|---:|
| /put?key=a | POST | 10B | 41077.79 | 0.4 ms | 2.1 ms | 10.9 ms | 200/20000 |
| /get?key=a | GET | - | 44050.68 | 0.4 ms | 2.1 ms | 11.1 ms | 200/20000 |

## YCSB-style mixes (tools/bench/http_workload.py)

Config: nkeys=10000, ops=20000, concurrency=50, value_size=64B

| Workload | Read/Write Mix | Throughput (ops/s) | avg (ms) | p50 (ms) | p95 (ms) | p99 (ms) | ok/err |
|---|---|---:|---:|---:|---:|---:|---:|
| A | 50/50 | 5272.51 | 7.48 | 5.69 | 12.99 | 35.98 | 20000/0 |
| B | 95/5 | 6203.70 | 6.29 | 4.85 | 10.99 | 33.20 | 20000/0 |
| C | 100/0 | 7671.78 | 5.30 | 2.64 | 8.25 | 36.76 | 19993/7 |

## HTTP Workload Driver (YCSB-style mixes)

Environment:
- Machine: (fill)
- OS: (fill)
- Build: (Debug/Release)
- Server: kv_http_server (cpp-httplib)
- Driver: tools/bench/http_workload.py (Python, urllib)

### Workload A (50% read / 50% write)
Command:
- python3 tools/bench/http_workload.py --preload --nkeys 10000 --ops 20000 --concurrency 50 --read_ratio 0.5 --value_size 64

Results:
- throughput: 5272.51 ops/sec
- latency (ms): avg 7.4785, p50 5.6924, p95 12.9924, p99 35.9789
- ok=20000, err=0

## Sync Policy Experiment (durability vs performance)

Config: base=http://127.0.0.1:8081, nkeys=10000, ops=20000, concurrency=50, value_size=64B

| Workload | read_ratio | sync_every | throughput (ops/s) | avg (ms) | p50 (ms) | p95 (ms) | p99 (ms) | ok/err |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| A | 0.5 | 1 | 6363.83 | 7.5895 | 6.2481 | 14.0794 | 38.0001 | 20000/0 |
| A | 0.5 | 100 | 5752.89 | 8.4642 | 7.2915 | 16.1942 | 30.0446 | 20000/0 |
| W | 0.1 | 1 | 4461.29 | 7.8950 | 4.5947 | 15.9883 | 65.6225 | 20000/0 |
| W | 0.1 | 100 | (run) | (run) | (run) | (run) | (run) | (run) |

## Sync Policy Experiment (durability vs performance)

Config: base=http://127.0.0.1:8081, nkeys=10000, ops=20000, concurrency=50, value_size=64B, driver=tools/bench/http_workload.py

| Workload | read_ratio | sync_every | throughput (ops/s) | avg (ms) | p50 (ms) | p95 (ms) | p99 (ms) | ok/err |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| W (write-heavy) | 0.1 | 1 | 4461.29 | 7.8950 | 4.5947 | 15.9883 | 65.6225 | 20000/0 |
| W (write-heavy) | 0.1 | 100 | 6297.00 | 7.6243 | 6.3008 | 13.9955 | 38.3852 | 20000/0 |