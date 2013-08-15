#include "iceberg_tests.h"
#include "ib_utils.h"
#include "test_ib_utils.h"

void test_expand_vars(ib::TestCase *c) {
  ib::string_map values;
  values["value"] = "1";
  std::string tpl("aaa bbb ${value}.");
  std::string result;
  ib::utils::expand_vars(result, tpl, values);
  ib_test_assert(strcmp(result.c_str(), "aaa bbb 1.") == 0, "");

  ib::string_map values1;
  values1["キー"] = "val";
  values1["hoge"] = "111";
  std::string tpl1("\\${キー}\\\\${キー} ${hoge} ${unknwon ${}");
  std::string result1;
  ib::utils::expand_vars(result1, tpl1, values1);
  ib_test_assert(strcmp(result1.c_str(), "${キー}\\val 111 ") == 0, "");
}

void test_parse_key_bind(ib::TestCase *c) {
  int result[3] = {};
  ib::utils::parse_key_bind(result, "ctrl-a");
  ib_test_assert(
      result[0] == FL_CTRL &&
      result[1] == (int)'a',
      "");
  memset(result, 0, sizeof(result));
  ib::utils::parse_key_bind(result, "ctrl-alt-backspace");
  ib_test_assert(
      result[0] == FL_CTRL &&
      result[1] == FL_ALT &&
      result[2] == FL_BackSpace,
      "");
}
