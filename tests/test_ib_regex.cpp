#include "iceberg_tests.h"
#include "ib_regex.h"
#include "test_ib_regex.h"

void test_regex_escape(ib::TestCase *c) {
  const char *text = "()[]";
  std::string escaped;
  ib::Regex::escape(escaped, text);
  ib_test_assert(escaped == "\\(\\)\\[\\]", "");

  // onig_set_default_case_fold_flag(INTERNAL_ONIGENC_CASE_FOLD_MULTI_CHAR);
  // causes segmentation fault when compiles the follow regex.
  ib::Regex re("\\(hist", ib::Regex::I);
  re.init();
}

void test_regex_match(ib::TestCase *c) {
  ib::Regex regex("hoge_(([a-z]+)_([0-9]+))", ib::Regex::NONE);
  regex.init();
  const char *str = "hoge_foo_123_あいう";
  ib_test_assert(regex.match(str) == 0, "");
  ib_test_assert(regex.getNumGroups() == 4, "");
  ib_test_assert(regex.getBytesMatched() == 12, "");

  std::string group1;
  regex._1(group1);
  ib_test_assert(strcmp("foo_123", group1.c_str()) == 0, "");

  std::string group2;
  regex._2(group2);
  ib_test_assert(strcmp("foo", group2.c_str()) == 0, "");

  std::string group3;
  regex._3(group3);
  ib_test_assert(strcmp("123", group3.c_str()) == 0, "");

  const char *str_not_matched = "1hoge_foo_123";
  ib_test_assert(regex.match(str_not_matched) != 0, "");
}

void test_regex_match_copy(ib::TestCase *c) {
  ib::Regex regex("hoge_(([a-z]+)_([0-9]+))", ib::Regex::NONE);
  regex.init();
  std::string *ptr = new std::string("hoge_foo_123_あいう");
  const char *str = ptr->c_str();
  ib_test_assert(regex.match(str) == 0, "");
  ib_test_assert(regex.getNumGroups() == 4, "");
  ib_test_assert(regex.getBytesMatched() == 12, "");
  regex.copyString(str);
  delete ptr;

  std::string group1;
  regex._1(group1);
  ib_test_assert(strcmp("foo_123", group1.c_str()) == 0, "");
}

void test_regex_search(ib::TestCase *c) {
  ib::Regex regex("hoge_([a-z]+)", ib::Regex::NONE);
  regex.init();
  const char *str = "hoge_foo";
  ib_test_assert(regex.search(str) == 0, "");
  ib_test_assert(regex.getNumGroups() == 2, "");

  std::string group1;
  regex.group(group1, 1);
  ib_test_assert(strcmp("foo", group1.c_str()) == 0, "");
}

void test_regex_search_utf8(ib::TestCase *c) {
  ib::Regex regex("(あ.)", ib::Regex::NONE);
  regex.init();
  const char *str = "んんあいんnんｱ";
  ib_test_assert(regex.search(str) == 0, "");
  ib_test_assert(regex.getNumGroups() == 2, "");

  std::string group1;
  regex.group(group1, 1);
  ib_test_assert(strcmp("あい", group1.c_str()) == 0, "");
}

void test_regex_search_pos(ib::TestCase *c) {
  ib::Regex regex("(あ.)", ib::Regex::NONE);
  regex.init();
  const char *str = "aaあいbbあうんあえｱ";

  ib_test_assert(regex.search(str, 0, 1) == 1, "");

  ib_test_assert(regex.search(str, 9) == 0, "");
  ib_test_assert(regex.getNumGroups() == 2, "");

  std::string group1;
  regex.group(group1, 1);
  ib_test_assert(strcmp("あう", group1.c_str()) == 0, "");
  ib_test_assert(regex.getLastpos() == 16, "");

  ib_test_assert(regex.search(str, regex.getLastpos()+1) == 0, "");
  ib_test_assert(regex.getNumGroups() == 2, "");

  std::string group2;
  regex.group(group2,1);
  ib_test_assert(strcmp("あえ", group2.c_str()) == 0, "");

}

void test_regex_split(ib::TestCase *c) {
  ib::Regex regex("\\s+", ib::Regex::NONE);
  regex.init();
  const char *str = "  aaa    bbb\tccc   ";
  std::vector<std::string*> result;
  regex.split(result, str);
  ib_test_assert(result.size() == 5, "");
  ib_test_assert(*(result.at(0)) == "", "");
  ib_test_assert(*(result.at(1)) == "aaa", "");
  ib_test_assert(*(result.at(2)) == "bbb", "");
  ib_test_assert(*(result.at(3)) == "ccc", "");
  ib_test_assert(*(result.at(4)) == "", "");

  ib::utils::delete_pointer_vectors(result);
}


void test_regex_gsub(ib::TestCase *c) {
  ib::Regex regex("hoge_(\\d+)_([あ-ん]+)", ib::Regex::NONE);
  regex.init();
  std::string result;
  regex.gsub(result, "hoge_123_あい_hoge_456_うえ_aaa", "\\1_replaced_\\2");
  ib_test_assert(result == "123_replaced_あい_456_replaced_うえ_aaa", "");

  ib::Regex regex1("hoge_(\\d+)_([あ-ん]+)", ib::Regex::NONE);
  regex1.init();
  std::string result1;
  regex1.gsub(result1, "hoge_123_あい_hoge_456_うえ_aaa", "\\1_replaced_\\2_I");
  ib_test_assert(result1 == "123_replaced_あい_I_456_replaced_うえ_I_aaa", "");
}

void test_regex_gsub_func(ib::TestCase *c) {
  ib::Regex regex("hoge_(\\d+)_([あ-ん]+)", ib::Regex::NONE);
  regex.init();
  std::string result;
  regex.gsub(result, "hoge_123_あい_hoge_456_うえ_aaa", 
    [](const ib::Regex &reg, std::string *buf, void *userdata){
      *buf += "replaced";
      buf->append(reg.getString() + reg.getStartpos(1), reg.getGroupLength(1));
    }, 0);
  ib_test_assert(result == "replaced123_replaced456_aaa", "");

}

