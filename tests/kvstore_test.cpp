#include "kvstore/kvstore.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <filesystem>


TEST(KVStoreTest, PutGetWorks) {
  kv::KVStore s;
  EXPECT_TRUE(s.Put("a", "1"));
  auto v = s.Get("a");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(*v, "1");
}

TEST(KVStoreTest, DeleteRemovesKey) {
  kv::KVStore s;
  s.Put("a", "1");
  EXPECT_TRUE(s.Del("a"));
  EXPECT_FALSE(s.Get("a").has_value());
  EXPECT_FALSE(s.Del("a"));
}

TEST(KVStoreTest, OverwriteUpdatesValue) {
  kv::KVStore s;
  s.Put("a", "1");
  s.Put("a", "2");
  auto v = s.Get("a");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(*v, "2");
}

TEST(KVStoreTest, MissingKeyReturnsNullopt) {
  kv::KVStore s;
  EXPECT_FALSE(s.Get("does_not_exist").has_value());
}

TEST(KVStoreTest, DeleteMissingKeyReturnsFalse) {
  kv::KVStore s;
  EXPECT_FALSE(s.Del("nope"));
}

TEST(KVStoreTest, PersistsAndRecoversFromLog) {
  const std::string path = "kvstore_test.aof";
  std::remove(path.c_str());

  {
    kv::KVStore s(path);
    s.Put("a", "1");
    s.Put("b", "hello");
    s.Del("a");
    s.Close();
  }

  {
    kv::KVStore s2(path);
    EXPECT_FALSE(s2.Get("a").has_value());
    auto vb = s2.Get("b");
    ASSERT_TRUE(vb.has_value());
    EXPECT_EQ(*vb, "hello");
  }

  std::remove(path.c_str());
}

TEST(KVStoreTest, RecoveryStopsSafelyOnTruncatedFinalRecord) {
  const std::string path = "kvstore_trunc_test.aof";
  std::remove(path.c_str());

  // Write a valid PUT record for key "good"
  {
    kv::KVStore s(path);
    s.Put("good", "ok");
    s.Close();
  }

  // Now append a *truncated* PUT record manually (simulate crash mid-write)
  // We claim value_size=5, but only write 2 bytes ("hi") and NO trailing newline.
  {
    std::ofstream out(path, std::ios::binary | std::ios::app);
    out << "PUT bad 5\n";
    out.write("hi", 2);
    out.flush();
  }

  // Recovery should keep "good" and ignore the partial "bad" record.
  {
    kv::KVStore s2(path);

    auto good = s2.Get("good");
    ASSERT_TRUE(good.has_value());
    EXPECT_EQ(*good, "ok");

    EXPECT_FALSE(s2.Get("bad").has_value());
  }

  std::remove(path.c_str());
}

TEST(KVStoreTest, PersistsAndRecoversWithIndex) {
  const std::string path = "kvstore_index_test.aof";
  std::remove(path.c_str());

  {
    kv::KVStore s(path);
    s.Put("a", "1");
    s.Put("b", "hello");
  }

  {
    kv::KVStore s2(path);
    auto a = s2.Get("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, "1");

    auto b = s2.Get("b");
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, "hello");
  }

  std::remove(path.c_str());
}

TEST(KVStoreTest, CompactionShrinksLogAndKeepsLatestValues) {
  namespace fs = std::filesystem;
  const std::string path = "kvstore_compact_test.aof";
  std::remove(path.c_str());

  kv::KVStore s(path);

  // Create lots of obsolete history
  for (int i = 0; i < 200; i++) {
    s.Put("hot", std::to_string(i));
  }
  s.Put("keep", "yes");
  s.Del("keep");
  s.Put("keep", "final");

  auto before = fs::file_size(path);

  ASSERT_TRUE(s.Compact());

  auto after = fs::file_size(path);

  // Correctness
  auto hot = s.Get("hot");
  ASSERT_TRUE(hot.has_value());
  EXPECT_EQ(*hot, "199");

  auto keep = s.Get("keep");
  ASSERT_TRUE(keep.has_value());
  EXPECT_EQ(*keep, "final");

  // File should be smaller after compaction
  EXPECT_LT(after, before);

  std::remove(path.c_str());
}

