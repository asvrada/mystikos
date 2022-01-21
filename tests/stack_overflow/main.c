// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int gettid()
{
    return syscall(SYS_gettid);
}

void sigsegv_handler()
{
    printf("[pid=%d] Stack overflow handled!\n", getpid());
    if (gettid() != getpid())
        pthread_exit(0);
    else
        exit(0);
}

void install_sigsegv_handler()
{
    // setup alt stack
    stack_t ss;
    void* stack;
    size_t stack_size;

    stack_size = SIGSTKSZ * 4; // 8 pages
    assert((stack = malloc(SIGSTKSZ * 4)) != NULL);

    ss.ss_size = stack_size;
    ss.ss_flags = 0;
    ss.ss_sp = stack;
    assert(sigaltstack(&ss, NULL) != -1);

    // test SS_DISABLE flag
    ss.ss_flags = SS_DISABLE;
    assert(sigaltstack(&ss, NULL) != -1);

    ss.ss_size = stack_size;
    ss.ss_flags = 0;
    ss.ss_sp = stack;
    assert(sigaltstack(&ss, NULL) != -1);

    // register sigsegv disposition
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sigsegv_handler;
    assert(sigaction(SIGSEGV, &sa, NULL) == 0);
}

void _so()
{
    char buf[4096];
    _so();
}

void* _thread_func(void* arg)
{
    install_sigsegv_handler();
    _so();
}

int main(int argc, const char* argv[])
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "test_main") == 0)
        {
            install_sigsegv_handler();
            _so();
        }
        else if (strcmp(argv[1], "test_child") == 0)
        {
            pid_t pid = fork();
            assert(pid != -1);

            if (pid)
            {
                waitpid(pid, NULL, 0);
                exit(0);
            }
            else if (pid == 0)
            {
                char* argVec[] = {"stk_ovf_test", "test_main", 0};
                char* envVec[] = {0};
                execve("/bin/stk_ovf_test", argVec, envVec);
            }
        }
        else if (strcmp(argv[1], "test_pthread") == 0)
        {
            pthread_t t;

            assert(pthread_create(&t, NULL, _thread_func, NULL) == 0);
            pthread_join(t, NULL);
        }
    }
    else
    {
        assert(0 && "invalid number of args");
    }
}
