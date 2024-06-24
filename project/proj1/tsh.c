/*
 * Copyright(c) 2023-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */

/*
 * 컴퓨터학부 2022027692 김채현
 * 표준 입출력 리다이렉션 및 파이프 기능 추가
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

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char* cmd)
{
    char* argv[MAX_LINE / 2 + 1];     /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;                            /* 인자의 개수 */
    char* p, * q;                            /* 명령어를 파싱하기 위한 변수 */

    int fd[2];                                   /* 파이프를 위한 배열 생성 */
    char cmds[MAX_LINE / 2 + 1];	    /* 파이프 명령어를 저장하기 위한 배열 */
    int cmdc = 0;		                        /* cmds 인자의 개수 */


    /*
     * 새로운 배열 cmds를 만든다.
     * 이 배열의 인자는 파이프 기호를 기준으로 나누어 저장한다.
     */
    
    //p = cmd, p += strspn(p, " \t");

    /*

    do {
        
        // 파이프 기호가 있는지 검사한다.
        q = strpbrk(p, "|");
        
        // 파이프가 있거나 아무것도 없으면 인자를 분리해서 cmds라는 새로운 배열에 집어넣는다.
        
        if (*q == "|" || q == NULL) {
            q = strsep(&p, "|");
            if (*q) cmds[cmdc++] = q;
        }
    } while (p);                    //포인터가 NULL일때까지 계속한다.
    cmds[cmdc] = NULL;      // 배열의 끝은 NULL로 처리해서 배열이 끝났다는 것을 명시해야함.

    */

    /*
    * 파이프 기능 구현
    * 2024년 4월 3일
    
    int in_fd = 0;		     // 입력 파일 디스크립터 
    int out_fd = 1;		 // 출력 파일 디스크립터 

    for (int i = 0; i < cmdc; i++) {

        if (i < cmdc - 1) {

            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        */


        /*
         * 명령어 앞부분 공백문자를 제거하고 인자에 차례로 저장한다.
         * 파이프의 모든 인자에 대해서 파싱을 실행하도록 한다.
         */
        //p = cmds[i], p += strspn(p, " \t");
        
    p = cmd, p += strspn(p, " \t");
        do {
            /*
             * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
             */
            q = strpbrk(p, " \t\'\"");
            /*
             * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
             */
            if (q == NULL || *q == ' ' || *q == '\t') {
                q = strsep(&p, " \t");
                if (*q) argv[argc++] = q;
            }

            /* 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
             * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
             * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
             */
            else if (*q == '\'') {
                q = strsep(&p, "\'");
                if (*q) argv[argc++] = q;
                q = strsep(&p, "\'");
                if (*q) argv[argc++] = q;
            }
            /*
             * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
             * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
             * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
             */
            else {
                q = strsep(&p, "\"");
                if (*q) argv[argc++] = q;
                q = strsep(&p, "\"");
                if (*q) argv[argc++] = q;
            }

        } while (p);
        argv[argc] = NULL;

    }


    /*
     * 표준 입출력 리다이렉션 기능
     * 2024년 3월 31일
     */

    for (int j = 0; j < argc; ++j) {
	    if (strcmp(argv[j], "<") == 0) {
		    in_fd = open(argv[j+1], O_RDONLY);
		    // 오류 처리
		    if (in_fd < 0) {
			    printf("file error!\n");
			    exit(EXIT_FAILURE);
		    }
		    dup2(in_fd, STDIN_FILENO); //파이프를 표준입력으로 설정
		    close(in_fd);
		    argv[j] = NULL;
	    } else if (strcmp(argv[j], ">") == 0) {
		    out_fd = open(argv[j+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
		    // 오류 처리
		    if (out_fd == -1) {
			    printf("file error!\n");
			    perror("file");
			    exit(EXIT_FAILURE);
		    }
		    dup2(out_fd, STDOUT_FILENO);
		    close(out_fd);
		    argv[j] = NULL;
		    
	    }
					    
			   
    }

    /*
    * 프로세스 생성
    pid_t pid;
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // 자식 프로세스
        close(fd[1]); // 쓰기 닫음
        dup2(fd[0], STDIN_FILENO); // 파이프를 표준 입력으로 설정
        close(fd[0]);
    }
    else { //부모 프로세스
        close(fd[0]); // 읽기 닫음
        dup2(fd[1], STDOUT_FILENO); // 파이프를 표준 출력으로 설정
        close(fd[1]);
    }
	  

      */

    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0) {
    execvp(argv[0], argv);
    }
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
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
