#include <stdio.h>
#include "CuTest.h"
    
    CuSuite* StrUtilGetSuite();
    
    void RunAllTests(void) {
        CuString *output = CuStringNew();
        CuSuite* suite = CuSuiteNew();
        
        CuSuiteAddSuite(suite, StrUtilGetSuite());
    
        CuSuiteRun(suite);
        CuSuiteSummary(suite, output);
        CuSuiteDetails(suite, output);
        printf("%s\n", output->buffer);
    }
    
    char *configFileName;

    int main(int argc, char *argv[]) {
      if (argc == 1) {
	configFileName="setest.conf";
      }
      else {
	configFileName=argv[1];
      }

      RunAllTests();
      return EXIT_SUCCESS;
    }

