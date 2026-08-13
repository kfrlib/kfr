#include "mini_catch.h"

TestCase* mc_tests_head          = NULL;
int mc_current_failures          = 0;
const char* mc_current_test_name = NULL;
jmp_buf mc_require_jump;

int main(void)
{
    int total_tests  = 0;
    int failed_tests = 0;
    for (TestCase* t = mc_tests_head; t; t = t->next)
    {
        total_tests++;
        mc_current_test_name = t->name;
        printf("[ RUN ] %s\n", t->name);
        mc_current_failures = 0;
        if (setjmp(mc_require_jump) == 0)
        {
            t->func();
        }
        if (mc_current_failures > 0)
        {
            failed_tests++;
            printf("[ FAIL ] %s\n", t->name);
        }
        else
        {
            printf("[ PASS ] %s\n", t->name);
        }
    }
    printf("\n%d test(s) run, %d failed.\n", total_tests, failed_tests);
    return failed_tests;
}
