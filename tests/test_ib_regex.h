#ifndef __IB_TEST_REGEX_H__
#define __IB_TEST_REGEX_H__
void test_regex_escape(ib::TestCase *c);
void test_regex_match(ib::TestCase *c);
void test_regex_match_copy(ib::TestCase *c);
void test_regex_search(ib::TestCase *c);
void test_regex_search_utf8(ib::TestCase *c);
void test_regex_search_pos(ib::TestCase *c);
void test_regex_split(ib::TestCase *c);
void test_regex_gsub(ib::TestCase *c);
void test_regex_gsub_func(ib::TestCase *c);


namespace ib {
  IB_TESTCASE(Regex)
    void build(){
      add(test_regex_escape);
      add(test_regex_match);
      add(test_regex_match_copy);
      add(test_regex_search);
      add(test_regex_search_utf8);
      add(test_regex_search_pos);
      add(test_regex_split);
      add(test_regex_gsub);
      add(test_regex_gsub_func);
    }
  IB_END_TESTCASE;
}
#endif
