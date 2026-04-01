#include "kvstore/kvstore.h"
#include "httplib.h"

#include <filesystem>
#include <iostream>
#include <string>

static int GetPort(int argc, char** argv) {
  int port = 8080;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
  }
  return port;
}

int main(int argc, char** argv) {
  int port = GetPort(argc, argv);

  std::filesystem::create_directories("data");
  kv::KVStore store("data/http.aof");

  httplib::Server svr;

  // POST /put?key=...  (body=value)
  svr.Post("/put", [&](const httplib::Request& req, httplib::Response& res) {
    auto key = req.get_param_value("key");
    if (key.empty()) {
      res.status = 400;
      res.set_content("missing key\n", "text/plain");
      return;
    }
    if (!store.Put(key, req.body)) {
      res.status = 500;
      res.set_content("put failed\n", "text/plain");
      return;
    }
    res.status = 200;
    res.set_content("OK\n", "text/plain");
  });

  // GET /get?key=...
  svr.Get("/get", [&](const httplib::Request& req, httplib::Response& res) {
    auto key = req.get_param_value("key");
    if (key.empty()) {
      res.status = 400;
      res.set_content("missing key\n", "text/plain");
      return;
    }
    auto v = store.Get(key);
    if (!v) {
      res.status = 404;
      res.set_content("(nil)\n", "text/plain");
      return;
    }
    res.status = 200;
    res.set_content(*v, "text/plain");
  });

  // POST /del?key=...
  svr.Post("/del", [&](const httplib::Request& req, httplib::Response& res) {
    auto key = req.get_param_value("key");
    if (key.empty()) {
      res.status = 400;
      res.set_content("missing key\n", "text/plain");
      return;
    }
    bool deleted = store.Del(key);
    res.status = 200;
    res.set_content(deleted ? "1\n" : "0\n", "text/plain");
  });

  // POST /compact
  svr.Post("/compact", [&](const httplib::Request&, httplib::Response& res) {
    if (!store.Compact()) {
      res.status = 500;
      res.set_content("compact failed\n", "text/plain");
      return;
    }
    res.status = 200;
    res.set_content("OK\n", "text/plain");
  });

  std::cout << "Listening on http://127.0.0.1:" << port << "\n";
  svr.listen("127.0.0.1", port);
  return 0;
}