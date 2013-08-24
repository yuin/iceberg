#ifndef __IB_TEST_PLATFORM_WIN_H__
#define __IB_TEST_PLATFORM_WIN_H__
void test_join_path(ib::TestCase *c);
void test_normalize_path(ib::TestCase *c);
void test_normalize_join_path(ib::TestCase *c);
void test_dirname(ib::TestCase *c);
void test_basename(ib::TestCase *c);
void test_is_directory(ib::TestCase *c);
void test_is_path(ib::TestCase *c);
void test_is_relative_path(ib::TestCase *c);
void test_directory_exists(ib::TestCase *c);
void test_file_exists(ib::TestCase *c);
void test_path_exists(ib::TestCase *c);
void test_walk_path(ib::TestCase *c);
void test_which(ib::TestCase *c);
void test_file_type(ib::TestCase *c);
void test_icon_cache_key(ib::TestCase *c);

namespace ib {
  IB_TESTCASE(PlatformWin)
    void build(){
      add(test_join_path);
      add(test_normalize_path);
      add(test_normalize_join_path);
      add(test_dirname);
      add(test_basename);
      add(test_directory_exists);
      add(test_file_exists);
      add(test_path_exists);
      add(test_is_directory);
      add(test_is_path);
      add(test_is_relative_path);
      add(test_walk_path);
      add(test_which);
      add(test_file_type);
      add(test_icon_cache_key);
    }
  IB_END_TESTCASE;
}
#endif
