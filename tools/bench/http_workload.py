#!/usr/bin/env python3
import argparse
import concurrent.futures as cf
import random
import statistics
import time
import urllib.request
import urllib.parse
from typing import List, Tuple


def http_get(url: str, timeout: float) -> Tuple[int, bytes]:
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.getcode(), resp.read()


def http_post(url: str, body: bytes, timeout: float) -> Tuple[int, bytes]:
    req = urllib.request.Request(url, data=body, method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.getcode(), resp.read()


def percentile(sorted_vals: List[float], p: float) -> float:
    if not sorted_vals:
        return 0.0
    if p <= 0:
        return sorted_vals[0]
    if p >= 1:
        return sorted_vals[-1]
    idx = p * (len(sorted_vals) - 1)
    lo = int(idx)
    hi = min(lo + 1, len(sorted_vals) - 1)
    frac = idx - lo
    return sorted_vals[lo] * (1.0 - frac) + sorted_vals[hi] * frac


def worker_thread(base: str, nkeys: int, nops: int, read_ratio: float, value: bytes,
                  timeout: float, seed: int) -> Tuple[int, int, List[float]]:
    rng = random.Random(seed)
    lat_ms: List[float] = []
    ok = 0
    err = 0

    for _ in range(nops):
        k = rng.randrange(nkeys)
        key = f"k{k}"

        is_read = (rng.random() < read_ratio)

        t0 = time.perf_counter()
        try:
            if is_read:
                url = f"{base}/get?key={urllib.parse.quote(key)}"
                code, _ = http_get(url, timeout=timeout)
                if code == 200:
                    ok += 1
                else:
                    err += 1
            else:
                url = f"{base}/put?key={urllib.parse.quote(key)}"
                code, _ = http_post(url, body=value, timeout=timeout)
                if code == 200:
                    ok += 1
                else:
                    err += 1
        except Exception:
            err += 1
        t1 = time.perf_counter()
        lat_ms.append((t1 - t0) * 1000.0)

    return ok, err, lat_ms


def preload(base: str, nkeys: int, value: bytes, timeout: float, concurrency: int) -> None:
    # Preload keys sequentially but with a small threadpool for speed.
    def put_one(i: int) -> int:
        key = f"k{i}"
        url = f"{base}/put?key={urllib.parse.quote(key)}"
        code, _ = http_post(url, body=value, timeout=timeout)
        return code

    with cf.ThreadPoolExecutor(max_workers=concurrency) as ex:
        futures = [ex.submit(put_one, i) for i in range(nkeys)]
        bad = 0
        for f in cf.as_completed(futures):
            code = f.result()
            if code != 200:
                bad += 1
        if bad:
            raise RuntimeError(f"Preload had {bad} non-200 responses")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="http://127.0.0.1:8080", help="Server base URL")
    ap.add_argument("--nkeys", type=int, default=10000, help="Number of keys (k0..kN-1)")
    ap.add_argument("--ops", type=int, default=20000, help="Total ops across all threads")
    ap.add_argument("--concurrency", type=int, default=50, help="Number of worker threads")
    ap.add_argument("--read_ratio", type=float, default=0.95, help="Fraction of GET ops (0..1)")
    ap.add_argument("--value_size", type=int, default=64, help="Value size in bytes")
    ap.add_argument("--timeout", type=float, default=2.0, help="HTTP timeout per request (seconds)")
    ap.add_argument("--preload", action="store_true", help="Preload all keys before running workload")
    ap.add_argument("--seed", type=int, default=12345, help="Random seed base")
    args = ap.parse_args()

    if args.nkeys <= 0 or args.ops <= 0 or args.concurrency <= 0:
        raise SystemExit("nkeys/ops/concurrency must be > 0")
    if not (0.0 <= args.read_ratio <= 1.0):
        raise SystemExit("read_ratio must be in [0,1]")
    if args.value_size < 0:
        raise SystemExit("value_size must be >= 0")

    value = (b"a" * args.value_size)

    if args.preload:
        print(f"Preloading {args.nkeys} keys...")
        preload(args.base, args.nkeys, value, timeout=args.timeout, concurrency=min(args.concurrency, 32))
        print("Preload done.")

    # Distribute ops across threads
    per = args.ops // args.concurrency
    rem = args.ops % args.concurrency

    print("Running workload:")
    print(f"  base={args.base} nkeys={args.nkeys} ops={args.ops} concurrency={args.concurrency}")
    print(f"  read_ratio={args.read_ratio} value_size={args.value_size} preload={args.preload}")

    start = time.perf_counter()

    all_lat: List[float] = []
    ok_total = 0
    err_total = 0

    with cf.ThreadPoolExecutor(max_workers=args.concurrency) as ex:
        futures = []
        for i in range(args.concurrency):
            nops = per + (1 if i < rem else 0)
            futures.append(
                ex.submit(
                    worker_thread,
                    args.base,
                    args.nkeys,
                    nops,
                    args.read_ratio,
                    value,
                    args.timeout,
                    args.seed + i,
                )
            )

        for f in cf.as_completed(futures):
            ok, err, lat = f.result()
            ok_total += ok
            err_total += err
            all_lat.extend(lat)

    end = time.perf_counter()
    total_s = end - start
    rps = args.ops / total_s if total_s > 0 else 0.0

    all_lat.sort()
    p50 = percentile(all_lat, 0.50)
    p95 = percentile(all_lat, 0.95)
    p99 = percentile(all_lat, 0.99)
    avg = statistics.mean(all_lat) if all_lat else 0.0

    print("\nResults:")
    print(f"  total_time_s={total_s:.4f}")
    print(f"  throughput_ops_per_s={rps:.2f}")
    print(f"  ok={ok_total} err={err_total}")
    print(f"  latency_ms_avg={avg:.4f} p50={p50:.4f} p95={p95:.4f} p99={p99:.4f}")


if __name__ == "__main__":
    main()