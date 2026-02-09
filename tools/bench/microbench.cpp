#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "kvstore/kvstore.h"

struct Args {
  int keys = 100000;
  int ops = 500000;
  int value_size = 64;
  double read_ratio = 0.8;   // fraction of ops that are GETs
  bool persistent = false;   // if true, uses data/bench.aof
};

static bool StartsWith(const std::string& s, const std::string& p) {
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

static Args ParseArgs(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; i++) {
    std::string x = argv[i];
    auto read_int = [&](const char* name, int& out) {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        std::exit(2);
      }
      out = std::stoi(argv[++i]);
    };
    auto read_double = [&](const char* name, double& out) {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        std::exit(2);
      }
      out = std::stod(argv[++i]);
    };

    if (x == "--keys") read_int("--keys", a.keys);
    else if (x == "--ops") read_int("--ops", a.ops);
    else if (x == "--value_size") read_int("--value_size", a.value_size);
    else if (x == "--read_ratio") read_double("--read_ratio", a.read_ratio);
    else if (x == "--persistent") a.persistent = true;
    else if (x == "--help" || x == "-h") {
      std::cout
        << "microbench options:\n"
        << "  --keys N          number of distinct keys (default 100000)\n"
        << "  --ops N           number of operations (default 500000)\n"
        << "  --value_size N    bytes per value (default 64)\n"
        << "  --read_ratio R    fraction GET ops in [0,1] (default 0.8)\n"
        << "  --persistent      use append-only log at data/bench.aof\n";
      std::exit(0);
    } else {
      std::cerr << "Unknown arg: " << x << "\n";
      std::exit(2);
    }
  }

  if (a.keys <= 0 || a.ops <= 0 || a.value_size < 0) {
    std::cerr << "Invalid args.\n";
    std::exit(2);
  }
  if (a.read_ratio < 0.0 || a.read_ratio > 1.0) {
    std::cerr << "--read_ratio must be in [0,1].\n";
    std::exit(2);
  }
  return a;
}

static std::string MakeKey(int i) {
  return "k" + std::to_string(i);
}

static std::string MakeValue(int size, std::mt19937& rng) {
  // Simple deterministic-ish payload
  std::string v;
  v.resize(size);
  std::uniform_int_distribution<int> dist(0, 25);
  for (int i = 0; i < size; i++) v[i] = char('a' + dist(rng));
  return v;
}

static double Percentile(std::vector<double>& xs, double p) {
  if (xs.empty()) return 0.0;
  std::sort(xs.begin(), xs.end());
  double idx = p * (xs.size() - 1);
  size_t lo = static_cast<size_t>(idx);
  size_t hi = std::min(lo + 1, xs.size() - 1);
  double frac = idx - lo;
  return xs[lo] * (1.0 - frac) + xs[hi] * frac;
}

int main(int argc, char** argv) {
  Args args = ParseArgs(argc, argv);

  // RNG setup
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> key_dist(0, args.keys - 1);
  std::uniform_real_distribution<double> op_dist(0.0, 1.0);

  // Create store
  std::unique_ptr<kv::KVStore> store;
  if (args.persistent) {
    // Ensure data directory exists (portable enough for mac)
    std::system("mkdir -p data >/dev/null 2>&1");
    std::system("rm -f data/bench.aof >/dev/null 2>&1");
    store = std::make_unique<kv::KVStore>("data/bench.aof");
  } else {
    store = std::make_unique<kv::KVStore>();
  }

  // Warmup: pre-fill some keys
  int warm = std::min(args.keys, 20000);
  for (int i = 0; i < warm; i++) {
    store->Put(MakeKey(i), MakeValue(args.value_size, rng));
  }

  std::vector<double> lat_us;
  lat_us.reserve(static_cast<size_t>(args.ops));

  auto t0 = std::chrono::steady_clock::now();

  for (int i = 0; i < args.ops; i++) {
    int k = key_dist(rng);
    std::string key = MakeKey(k);

    bool is_read = (op_dist(rng) < args.read_ratio);

    auto op_start = std::chrono::steady_clock::now();

    if (is_read) {
      (void)store->Get(key);
    } else {
      store->Put(key, MakeValue(args.value_size, rng));
    }

    auto op_end = std::chrono::steady_clock::now();
    double us = std::chrono::duration<double, std::micro>(op_end - op_start).count();
    lat_us.push_back(us);
  }

  auto t1 = std::chrono::steady_clock::now();
  double total_s = std::chrono::duration<double>(t1 - t0).count();
  double ops_per_s = args.ops / total_s;

  double p50 = Percentile(lat_us, 0.50);
  double p95 = Percentile(lat_us, 0.95);
  double p99 = Percentile(lat_us, 0.99);

  std::cout << "microbench results\n";
  std::cout << "  keys=" << args.keys
            << " ops=" << args.ops
            << " value_size=" << args.value_size
            << " read_ratio=" << args.read_ratio
            << " persistent=" << (args.persistent ? "true" : "false") << "\n";
  std::cout << "  total_time_s=" << total_s << "\n";
  std::cout << "  throughput_ops_per_s=" << ops_per_s << "\n";
  std::cout << "  latency_us_p50=" << p50 << " p95=" << p95 << " p99=" << p99 << "\n";

  return 0;
}
