#ifndef __IB_TEST_UTILS_H__
#define __IB_TEST_UTILS_H__
void test_expand_vars(ib::TestCase *c);
void test_parse_key_bind(ib::TestCase *c);

namespace ib {
  IB_TESTCASE(Utils)
    void build(){
      add(test_expand_vars);
      add(test_parse_key_bind);
    }
  IB_END_TESTCASE;
}
#endif
