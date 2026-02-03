#include "kvstore/kvstore.h"
#include <gtest/gtest.h>
#include <cstdio>   // std::remove

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
