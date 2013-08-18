#include "iceberg_tests.h"
#include "ib_platform.h"
#include "test_ib_platform_win.h"

void test_join_path(ib::TestCase *c){
  ib::unique_oschar_ptr result1(ib::platform::join_path(0, L"c:\\hoge\\aa", L"bb.txt"));
  ib_test_assert(_tcscmp(result1.get(), L"c:\\hoge\\aa\\bb.txt") == 0, "");

  ib::unique_oschar_ptr result2(ib::platform::join_path(0, L"c:\\hoge\\aa\\", L"bb.txt"));
  ib_test_assert(_tcscmp(result2.get(), L"c:\\hoge\\aa\\bb.txt") == 0, "");

  ib::unique_oschar_ptr result3(ib::platform::join_path(0, L"c:/hoge/aa/", L"bb.txt"));
  ib_test_assert(_tcscmp(result3.get(), L"c:/hoge/aa/bb.txt") == 0, "");

  ib::unique_oschar_ptr result4(ib::platform::join_path(0, L".", L"bb.txt"));
  ib::unique_char_ptr tmp(ib::platform::oschar2utf8(result4.get()));
  ib_test_assert(_tcscmp(result4.get(), L".\\bb.txt") == 0, "");
}

void test_normalize_path(ib::TestCase *c){
  ib::unique_oschar_ptr result1(ib::platform::normalize_path(0, L"c:/hoge\\../a\\b.txt"));
  ib_test_assert(_tcscmp(result1.get(), L"c:\\a\\b.txt") == 0, "");

  ib::unique_oschar_ptr result2(ib::platform::normalize_path(0, L"..\\hoge\\foo"));
  ib_test_assert(_tcscmp(result2.get(), L"..\\hoge\\foo") == 0, "");

  ib::unique_oschar_ptr result3(ib::platform::normalize_path(0, L".\\hoge\\foo"));
  ib_test_assert(_tcscmp(result3.get(), L".\\hoge\\foo") == 0, "");

  ib::unique_oschar_ptr result4(ib::platform::normalize_path(0, L"../hoge\\foo"));
  ib_test_assert(_tcscmp(result4.get(), L"..\\hoge\\foo") == 0, "");

  ib::unique_oschar_ptr result5(ib::platform::normalize_path(0, L"./hoge\\foo"));
  ib_test_assert(_tcscmp(result5.get(), L".\\hoge\\foo") == 0, "");
}

void test_normalize_join_path(ib::TestCase *c){
  ib::unique_oschar_ptr result1(ib::platform::normalize_join_path(0, L"c:/hoge/aa/", L"bb.txt"));
  ib_test_assert(_tcscmp(result1.get(), L"c:\\hoge\\aa\\bb.txt") == 0, "");

  ib::unique_oschar_ptr result2(ib::platform::normalize_join_path(0, L"c:/hoge/aa/", L"bb.txt"));
  ib_test_assert(_tcscmp(result2.get(), L"c:\\hoge\\aa\\bb.txt") == 0, "");
}

void test_dirname(ib::TestCase *c){
  ib::unique_oschar_ptr result1(ib::platform::dirname(0, L"c:\\hoge\\aa\\"));
  ib_test_assert(_tcscmp(result1.get(), L"c:\\hoge\\aa") == 0, "");

  ib::unique_oschar_ptr result2(ib::platform::dirname(0, L"c:\\hoge\\aa\\file.txt"));
  ib_test_assert(_tcscmp(result2.get(), L"c:\\hoge\\aa") == 0, "");

  ib::unique_oschar_ptr result3(ib::platform::dirname(0, L"c:\\hoge\\aa/"));
  ib_test_assert(_tcscmp(result3.get(), L"c:\\hoge\\aa") == 0, "");

  ib::unique_oschar_ptr result4(ib::platform::dirname(0, L"c:\\hoge\\aa/file.txt"));
  ib_test_assert(_tcscmp(result4.get(), L"c:\\hoge\\aa") == 0, "");

  ib::unique_oschar_ptr result5(ib::platform::dirname(0, L"file.txt"));
  ib_test_assert(_tcscmp(result5.get(), L"") == 0, "");

  ib::unique_oschar_ptr result6(ib::platform::dirname(0, L"..\\"));
  ib_test_assert(_tcscmp(result6.get(), L"..") == 0, "");

  ib::unique_oschar_ptr result7(ib::platform::dirname(0, L".\\"));
  ib_test_assert(_tcscmp(result7.get(), L".") == 0, "");

  ib::unique_oschar_ptr result8(ib::platform::dirname(0, L"..\\hoge"));
  ib_test_assert(_tcscmp(result8.get(), L"..") == 0, "");

  ib::unique_oschar_ptr result9(ib::platform::dirname(0, L"\\\\"));
  ib_test_assert(_tcscmp(result9.get(), L"\\\\") == 0, "");

  ib::unique_oschar_ptr result10(ib::platform::dirname(0, L"\\\\hogehoge"));
  ib_test_assert(_tcscmp(result10.get(), L"\\\\") == 0, "");

  ib::unique_oschar_ptr result11(ib::platform::dirname(0, L"\\\\hogehoge\\"));
  ib_test_assert(_tcscmp(result11.get(), L"\\\\hogehoge") == 0, "");
}

void test_basename(ib::TestCase *c){
  ib::oschar result[IB_MAX_PATH];
  ib::platform::basename(result, L"c:\\hoge\\aa\\");
  ib_test_assert(_tcscmp(result, L"") == 0, "");

  ib::platform::basename(result, L"c:\\hoge\\aa\\file.txt");
  ib_test_assert(_tcscmp(result, L"file.txt") == 0, "");

  ib::platform::basename(result, L"c:\\hoge\\aa/");
  ib_test_assert(_tcscmp(result, L"") == 0, "");

  ib::platform::basename(result, L"c:\\hoge\\aa/file.txt");
  ib_test_assert(_tcscmp(result, L"file.txt") == 0, "");

  ib::platform::basename(result, L"..\\hoge");
  ib_test_assert(_tcscmp(result, L"hoge") == 0, "");
}

void test_is_directory(ib::TestCase *c){
  ib_test_assert(ib::platform::is_directory(L"c:\\WINDOWS"), "");
  ib_test_assert(ib::platform::is_directory(L"c:\\___hoge___\\"), "");
  ib_test_assert(!ib::platform::is_directory(L"c:\\WINDOWS\\aaaaa"), "");
}

void test_is_path(ib::TestCase *c){
  ib_test_assert(ib::platform::is_path(L"c:\\hoge"), "");
  ib_test_assert(ib::platform::is_path(L"c:/hoge"), "");
  ib_test_assert(ib::platform::is_path(L"Z:/hoge"), "");
  ib_test_assert(ib::platform::is_path(L"Z:/"), "");
  ib_test_assert(ib::platform::is_path(L"Z:\\"), "");
  ib_test_assert(ib::platform::is_path(L"..\\"), "");
  ib_test_assert(ib::platform::is_path(L".\\"), "");
  ib_test_assert(ib::platform::is_path(L"..\\hoge"), "");
  ib_test_assert(ib::platform::is_path(L".\\c"), "");
}

void test_is_relative_path(ib::TestCase *c){
  ib_test_assert(!ib::platform::is_relative_path(L"c:\\hoge"), "");
  ib_test_assert(!ib::platform::is_relative_path(L"c:/hoge"), "");
  ib_test_assert(!ib::platform::is_relative_path(L"Z:/hoge"), "");
  ib_test_assert(!ib::platform::is_relative_path(L"Z:/"), "");
  ib_test_assert(!ib::platform::is_relative_path(L"Z:\\"), "");
  ib_test_assert(ib::platform::is_relative_path(L"..\\"), "");
  ib_test_assert(ib::platform::is_relative_path(L".\\"), "");
  ib_test_assert(ib::platform::is_relative_path(L"..\\hoge"), "");
  ib_test_assert(ib::platform::is_relative_path(L".\\c"), "");
}

void test_directory_exists(ib::TestCase *c){
  ib_test_assert(ib::platform::directory_exists(L"c:\\WINDOWS"), "");
  ib_test_assert(!ib::platform::directory_exists(L"c:\\WINDOWS\\notepad.exe"), "");
}

void test_file_exists(ib::TestCase *c){
  ib_test_assert(!ib::platform::file_exists(L"c:\\WINDOWS"), "");
  ib_test_assert(ib::platform::file_exists(L"c:\\WINDOWS\\notepad.exe"), "");
}

void test_path_exists(ib::TestCase *c){
  ib_test_assert(ib::platform::path_exists(L"c:\\WINDOWS"), "");
  ib_test_assert(ib::platform::path_exists(L"c:\\WINDOWS\\notepad.exe"), "");
}

void test_walk_path(ib::TestCase *c){
  ib::Error error;
  std::vector<ib::unique_oschar_ptr> result;
  ib::platform::walk_dir(result, L"C:\\a", error, false);
  int i = 0;
  std::cout << "\n ******* walk_path ******* " << std::endl;
  std::cout << " --- flat --- " << std::endl;
  for(auto it = result.begin(), last = result.end(); it != last; ++it, ++i){
    wprintf(L"%d:%ls\n", i, (*it).get());
  }

  std::vector<ib::unique_oschar_ptr> result1;
  std::cout << "\n --- recursive --- " << std::endl;
  ib::platform::walk_dir(result1, L"C:\\a", error, true);
  i = 0;
  for(auto it = result1.begin(), last = result1.end(); it != last; ++it, ++i){
    wprintf(L"%d:%ls\n", i, (*it).get());
  }
}

void test_which(ib::TestCase *c){
  ib::oschar result[IB_MAX_PATH];

  wsprintf(result, L"");
  ib_test_assert(ib::platform::which(result, L"taskmgr.exe"), "");
  ib_test_assert(_tcscmp(L"c:\\windows\\system32\\taskmgr.exe", result) == 0, "");
  wsprintf(result, L"");
  ib_test_assert(ib::platform::which(result, L"cmd"), "");
  ib_test_assert(_tcscmp(L"c:\\windows\\system32\\cmd.EXE", result) == 0, "");
}
