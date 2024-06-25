/*
 * Copyright(c) 2023-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */


// Function declarations
void cmdexec(char *cmd);
void handle_redirection(char *p);
void parse_arguments(char *cmd, char *argv[], int *argc);
void execute_pipe(char *first, char *other);

// Handle input/output redirection
void handle_redirection(char *p) {
    char *input, *output;
    char *q = strpbrk(p, "<>");
    if (*q == '<') {
        p = strsep(&q, "<");
        p += strspn(p, " \t");
        if (*p) {
            input = p;
            int fd = open(input, O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
    } else if (*q == '>') {
        p = strsep(&q, ">");
        p += strspn(p, " \t");
        if (*p) {
            output = p;
            int fd = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }
}

// Parse command arguments
void parse_arguments(char *cmd, char *argv[], int *argc) {
    char *p = cmd;
    char *q;
    *argc = 0;

    do {
        q = strpbrk(p, " \t\'\"<>");

        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) argv[(*argc)++] = q;
        } else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[(*argc)++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[(*argc)++] = q;
        } else if (*q == '\"') {
            q = strsep(&p, "\"");
            if (*q) argv[(*argc)++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[(*argc)++] = q;
        } else if (*q == '<' || *q == '>') {
            handle_redirection(p);
            break;
        }
    } while (p);

    argv[*argc] = NULL;
}

// Execute command with pipe
void execute_pipe(char *first, char *other) {
    int fd[2];
    pid_t pid;

    if (pipe(fd) == -1) {
        fprintf(stderr, "Pipe failed\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        cmdexec(first);
    } else {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        cmdexec(other);
    }
}

// Execute the given command
void cmdexec(char *cmd) {
    char *argv[MAX_LINE / 2 + 1];
    int argc = 0;
    char *p, *first, *other;

    p = cmd;
    p += strspn(p, " \t");

    if (strchr(p, '|') != NULL) {
        first = strsep(&p, "|");
        other = p;
        execute_pipe(first, other);
        return;
    }

    parse_arguments(cmd, argv, &argc);

    if (argc > 0) {
        if (execvp(argv[0], argv) == -1) {
            printf("%s: 명령을 찾을 수 없습니다\n", argv[0]);
        }
    }
}
/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void) {
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    bool background;            /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = true;
            *p = '\0';
        }
        else
            background = false;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
