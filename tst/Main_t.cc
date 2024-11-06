#include <gtest/gtest.h>

using namespace std;

int main(int argc, char *argv[]) 
{
  int rc{};

  // First, pass arguments to Gtest
  // it will catch args such as --gtest_filter=Log_test.Base
  ::testing::InitGoogleTest(&argc, argv);
  rc = RUN_ALL_TESTS();
  return rc;
}



