#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#define EXERCISE_CHAPTER 0
#define EXERCISE_NUM 1

#define LINE_END "\x1b[0m\n"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN "\x1b[32m"
#define CHECK "\xE2\x9C\x93"
#define CROSS "\xE2\x9C\x96"

typedef bool(*bool_func)();

// Utils

void util_makeExeDirPath (char *path) {
    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
    *strrchr(path, '/') = '\0';
    strcat(path, "/..");
}

// Tests section

bool test_compile () {
    char path[PATH_MAX];
    util_makeExeDirPath(path);
    char make_cmd[4123];
    snprintf(make_cmd, 4123, "make -s -B -C \"%s\" " BUILD_NAME, path);
    int makeStatus = system(make_cmd);
    if (makeStatus != 0) {
        printf(COLOR_RED CROSS " Failed to compile program with make." LINE_END);
        return false;
    }
    return true;
}

bool test_status () {
    char path[PATH_MAX];
    util_makeExeDirPath(path);
    char exe_cmd[4123];
    snprintf(exe_cmd, 4123, "\"%s/main\" >/dev/null", path);
    int status = system(exe_cmd);
    if (status != 0) {
        printf(COLOR_RED CROSS " The program does not return status code 0." LINE_END, status);
        return false;
    }
    return status == 0;
}

bool test_stdout () {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        printf(COLOR_RED CROSS " Couldn't initialize stdout pipe. Abort." LINE_END);
        return false;
    }
    pid_t pid = fork();
    if (pid == -1) {
        printf(COLOR_RED CROSS " Couldn't initialize stdout fork. Abort." LINE_END);
        return false;
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char path[PATH_MAX];
        util_makeExeDirPath(path);
        char exe_cmd[4123];
        snprintf(exe_cmd, 4123, "%s/main", path);
        execl(exe_cmd, NULL);
        perror("execl");
        _exit(127);
    }
    close(pipefd[1]);

    char expected_stdout[] = "Hello World!\n";
    bool matches = true;
    for (int i = 0; i < sizeof(expected_stdout) / sizeof(char) - 1; i++) {
        char cur;
        read(pipefd[0], &cur, 1);
        if (cur != expected_stdout[i]) {
            printf(COLOR_RED CROSS " Your program does not print the expected string." LINE_END);
            matches = false;
            break;
        }
    }
    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    return matches;
}

// Running section

typedef struct {
    const char *name;
    bool_func func;
    bool stopIfFail;
} Test;

Test tests[] = {
    {"Compile", test_compile, true},
    {"ReturnsSuccessStatus", test_status, false},
    {"PrintsHelloWorld", test_stdout, false}
};

int main () {
    printf("Testing your implementation of exercise %d.%d...\n\n", EXERCISE_CHAPTER, EXERCISE_NUM);
    unsigned int totalTests = sizeof(tests) / sizeof(Test);
    unsigned int successfulTests = 0;
    chdir("..");
    for (int i = 0; i < totalTests; i++) {
        Test curTest = tests[i];
        printf("Running test %d: %s\n", i, curTest.name);
        if (curTest.func()) {
            successfulTests++;
            printf(COLOR_GREEN CHECK " Success!" LINE_END);
        } else if (curTest.stopIfFail) {
            printf(COLOR_RED "Aborted." LINE_END "\n");
            break;
        }
        printf("\n");
    }
    float success = successfulTests * 100.0f / totalTests;
    if (success < 40.0f) 
        printf(COLOR_RED);
    else if (success < 80.0f) 
        printf(COLOR_YELLOW);
    else 
        printf(COLOR_GREEN);
    printf("\n%.1f%% success [%d/%d tests succeeded]" LINE_END, success, successfulTests, totalTests);
    printf("[Press Enter to quit.]");
    getchar();
    return 0;
}