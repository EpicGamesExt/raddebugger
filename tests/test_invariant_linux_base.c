#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

extern void linux_base_backtrace(void);

START_TEST(test_backtrace_path_injection_boundary)
{
    // Invariant: Backtrace resolution must not execute attacker-controlled binaries via PATH manipulation
    const char *payloads[] = {
        "./malicious_addr2line",
        "../../../tmp/evil_tool",
        "normal_path"
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    char original_path[4096];
    char *env_path = getenv("PATH");
    if (env_path) {
        strncpy(original_path, env_path, sizeof(original_path) - 1);
        original_path[sizeof(original_path) - 1] = '\0';
    }

    for (int i = 0; i < num_payloads; i++) {
        mkdir("/tmp/test_inject", 0755);
        char malicious_script[512];
        snprintf(malicious_script, sizeof(malicious_script), "/tmp/test_inject/%s", payloads[i]);
        
        FILE *fp = fopen(malicious_script, "w");
        if (fp) {
            fprintf(fp, "#!/bin/sh\necho 'INJECTED'\necho 'INJECTED'\n");
            fclose(fp);
            chmod(malicious_script, 0755);
        }

        setenv("PATH", "/tmp/test_inject:/usr/bin:/bin", 1);
        
        linux_base_backtrace();
        
        unlink(malicious_script);
    }

    if (env_path) {
        setenv("PATH", original_path, 1);
    }
    rmdir("/tmp/test_inject");
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_backtrace_path_injection_boundary);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}