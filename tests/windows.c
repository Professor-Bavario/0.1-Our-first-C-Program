#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

#define EXERCISE_CHAPTER 0
#define EXERCISE_NUM 1

#define LINE_END "\x1b[0m\n"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN "\x1b[32m"
#define CHECK "\xE2\x9C\x93"
#define CROSS "\xE2\x9C\x96"

#ifdef VS
    #define NEWLINE "\r\n"
#else
    #define NEWLINE "\n"
#endif

typedef bool(*bool_func)();

// Utils

void util_makeExeDirPath (char *path) {
    GetModuleFileNameA(NULL, path, MAX_PATH);
    *strrchr(path, '\\') = '\0';
    strcat(path, "\\..");
}

// Tests section

bool test_compile () {
    char path[MAX_PATH];
    util_makeExeDirPath(path);
    char make_cmd[512];
    #ifdef VS
        snprintf(make_cmd, 512, "cd \"%s\" ; nmake " BUILD_NAME, path);
    #else
        snprintf(make_cmd, 512, "make -s -B -C \"%s\" " BUILD_NAME, path);
    #endif
    printf("%s\n", make_cmd);
    int makeStatus = system(make_cmd);
    if (makeStatus != 0) {
        printf(COLOR_RED CROSS " Failed to compile program with make." LINE_END);
        return false;
    }
    return true;
}

bool test_status () {
    char path[MAX_PATH];
    util_makeExeDirPath(path);
    char exe_cmd[512];
    snprintf(exe_cmd, 512, "\"%s\\main.exe\" >/dev/null", path);
    int status = system(exe_cmd);
    if (status != 0) {
        printf(COLOR_RED CROSS " The program does not return status code 0." LINE_END, status);
        return false;
    }
    return status == 0;
}

bool test_stdout () {
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = NULL;
    HANDLE writePipe = NULL;

    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
        printf(COLOR_RED CROSS " Couldn't initialize stdout pipe. Abort." LINE_END);
        return false;
    }
    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        printf(COLOR_RED CROSS " Couldn't configure pipe inheritance for stdout. Abort." LINE_END);
        return false;
    }
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    
    char path[MAX_PATH];
    util_makeExeDirPath(path);
    char exe_cmd[512];
    snprintf(exe_cmd, 512, "\"%s\\main.exe\"", path);
    if (!CreateProcessA(NULL, exe_cmd, NULL, NULL, true, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        printf(COLOR_RED CROSS " Couldn't start main.exe. Abort." LINE_END);
        return false;
    }
    CloseHandle(writePipe);

    char expected_stdout[] = "Hello World!" NEWLINE;
    bool matches = true;
    for (int i = 0; i < sizeof(expected_stdout) / sizeof(char) - 1; i++) {
        char cur;
        ReadFile(readPipe, &cur, 1, NULL, NULL);
        if (cur != expected_stdout[i]) {
            printf(COLOR_RED CROSS " Your program does not print the expected string." LINE_END);
            matches = false;
            break;
        }
    }
    WaitForSingleObject(pi.hProcess, 1000);

    CloseHandle(readPipe);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
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