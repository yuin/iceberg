#ifndef __IB_TESTS_H__
#define __IB_TESTS_H__

#include "ib_constants.h"
#include "ib_utils.h"

#define IB_TESTCASE(name) \
  class Test##name : public ib::TestCase {\
    public: \
      Test##name(ib::TestSuite *s) : ib::TestCase(s) {}
#define IB_END_TESTCASE };
#define ib_test_assert(cond, message) \
  if(!(cond)){ \
    c->getSuite()->addResult(new ib::TestResult(__FILE__, __LINE__, false, message)); \
  }else{ \
    c->getSuite()->addResult(new ib::TestResult(__FILE__, __LINE__, true, "")); \
  }
#define BEGIN_TEST_LEAKING long malloc_count = ib::utils::malloc_count();
#define END_TEST_LEAKING ib_test_assert(malloc_count == ib::utils::malloc_count(), "memory leak!");

namespace ib { 
  class TestResult : private ib::NonCopyable<TestResult> {
    protected:
      std::string filename_;
      int         line_;
      bool        success_;
      std::string message_;
    public:
      TestResult(const std::string &filename, int line, bool success, const std::string &message) : filename_(filename), line_(line), success_(success), message_(message) {};
      const std::string& getFilename() const { return filename_; }
      const int getLine() const { return line_; }
      const bool isSuccess() const { return success_; }
      const std::string& getMessage() { 
        if(message_ == "") { message_ += "Assertion failed."; }
        return message_; 
      }
      void setMessage(const std::string &value){ message_ = value; }
  };

  class TestSuite;
  class TestCase;
  typedef void (*testfunc)(TestCase*);

  class TestCase : private ib::NonCopyable<TestCase> {
    protected:
      TestSuite* s_;
      std::vector<ib::testfunc> tests_;
    public:
      TestCase(TestSuite* s) : s_(s), tests_() {}
      TestSuite* getSuite() { return s_;}
      void add(ib::testfunc func) { tests_.push_back(func); }
      void run();
      virtual void build() = 0;
      virtual void setup();
      virtual void teardown();
  };

  class TestReporter : private ib::NonCopyable<TestReporter>{
    public:
      virtual void onProgress(TestSuite *s, TestResult *result) = 0;
      virtual void onFinish(TestSuite *s) = 0;
  };

  class SimpleTestReporter : public TestReporter {
      void onProgress(TestSuite *s, TestResult *result);
      void onFinish(TestSuite *s);
  };

  class TestSuite : private ib::NonCopyable<TestSuite> {
    protected:
      std::vector<TestResult*> results_;
      std::vector<TestCase*>   test_cases_;
      ib::TestReporter *reporter_;
    public:
      TestSuite(ib::TestReporter *reporter) : results_(), test_cases_(), reporter_(reporter) {}
      virtual ~TestSuite() {
        ib::utils::delete_pointer_vectors(results_);
        ib::utils::delete_pointer_vectors(test_cases_);
      }
      void add(TestCase* value) { test_cases_.push_back(value); }
      std::vector<TestResult*>& getResults() { return results_; }
      void run(char const *argv[]);
      void addResult(TestResult *value) { 
        results_.push_back(value); 
        reporter_->onProgress(this, value);
      }
      virtual void build() = 0;
      virtual void startup();
      virtual void shutdown();
  };

}

#endif
