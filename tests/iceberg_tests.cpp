#include "iceberg_tests.h"
#include "test_ib_utils.h"
#include "test_ib_platform_win.h"
#include "test_ib_regex.h"

// {{{
void ib::TestCase::run(){
  build();
  for(auto it = tests_.begin(), last = tests_.end(); it != last; ++it){
    setup();
    (*it)(this);
    teardown();
  }
}
void ib::TestCase::setup() {}
void ib::TestCase::teardown() {}

void ib::TestSuite::run(char const *argv[]) {
  build();
  startup();
  for(auto it = test_cases_.begin(), last = test_cases_.end(); it != last; ++it){
    (*it)->run();
  }
  shutdown();
  reporter_->onFinish(this);
}

void ib::TestSuite::startup() {}
void ib::TestSuite::shutdown() {}

void ib::SimpleTestReporter::onProgress(TestSuite *s, TestResult *result){
  if(result->isSuccess()) {
    std::cout << "." << std::flush;
  }else{
    std::cout << "F" << std::flush;
  }
}

void ib::SimpleTestReporter::onFinish(TestSuite *s){
  unsigned long success = 0, fail = 0;
  std::cout << std::endl;
  std::cout << std::endl;
  std::vector<ib::TestResult*> &results = s->getResults();
  for(auto it = results.begin(), last = results.end(); it != last; ++it){
    if(!(*it)->isSuccess()) {
      std::cout << (*it)->getFilename() << ", line " << (*it)->getLine() << ":" << (*it)->getMessage() << std::endl;
      fail++;
    }else{
      success++;
    }
  }
  std::cout << "====================Summary===================" << std::endl;
  if(fail == 0) {
    std::cout << "conguraturations!" << std::endl;
  }else{
    std::cout << success << " passed, " << fail << " failed." << std::endl;
  }

}
// }}}

class AllTests : public ib::TestSuite {
  public:
    AllTests(ib::TestReporter *reporter) : TestSuite(reporter) {}
    void build() {
      add(new ib::TestUtils(this));
      add(new ib::TestPlatformWin(this));
      add(new ib::TestRegex(this));
    }
};

int main (int argc, char const* argv[]) {
  long malloc_count = ib::utils::malloc_count();
  {
    std::unique_ptr<ib::TestReporter> reporter(new ib::SimpleTestReporter());
    AllTests tests(reporter.get());
    tests.run(argv);
  }
  if(malloc_count != ib::utils::malloc_count()) {
    std::cout << "************ memory leak!(" << ib::utils::malloc_count() << ") *****************" << std::endl;
  }
  return 0;
}
