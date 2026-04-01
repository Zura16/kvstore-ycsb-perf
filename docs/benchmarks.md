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