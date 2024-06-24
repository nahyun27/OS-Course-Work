/*
 * Copyright(c) 2021-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 학과: 컴퓨터학부
 * 학번: 2019055014
 * 이름: 김민수
 * 수정 내용: spinlock, 유한 대기 구현 및 usleep 시간 수정
 * 수정 날짜: 2024.05.02
 */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 8             /* 스레드 개수 */
#define RUNTIME 100000  /* 출력량을 제한하기 위한 실행시간 (마이크로초) */

/*
 * ANSI 컬러 코드: 출력을 쉽게 구분하기 위해서 사용한다.
 * 순서대로 BLK, RED, GRN, YEL, BLU, MAG, CYN, WHT, RESET을 의미한다.
 */
char *color[N+1] = {"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;34m","\e[0;35m","\e[0;36m","\e[0;37m","\e[0m"};

/*
 * waiting[i]는 스레드 i가 임계구역에 들어가기 위해 기다리고 있음을 나타낸다.
 * alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다.
 */
bool waiting[N];
bool alive = true;

/*
 * lock은 spinlock을 구현하기 위해 사용할 변수이다.
 */
atomic_int lock = 0;
/*
 * N 개의 스레드가 임계구역에 배타적으로 들어가기 위해 스핀락을 사용하여 동기화한다.
 */
void *worker(void *arg)
{
    int i = *(int *)arg;
    int j; // j는 임계구역에서 작업을 마친 뒤 lock을 넘겨줄 스레드를 찾기 위한 변수이다.
    while (alive) {

        waiting[i] = true;
        int expected = 0;
        /*
         * 임계구역에 들어가기 전에 스핀락을 걸어 상호배타를 보장할 수 있도록 한다.
         * 임계구역에 들어가기 위해서는 다른 스레드로부터 lock을 넘겨받아 waiting[i]가
         * false로 바뀌었거나 lock이 걸려있지 않아야 한다. 그렇지 않으면 계속 대기한다.
         * lock이 0이면 1로 바꾸고 루프를 빠져나오게 되지만 그렇지 않은 경우는 expected가
         * 1로 바뀌기 때문에 다시 0으로 바꾸고 루프를 돌려 대기하도록 한다.
         */

        while(waiting[i] && !atomic_compare_exchange_weak(&lock, &expected, 1)) {
            expected = 0;
        }

        waiting[i] = false; // 스레드 i가 임계구역에 진입했으므로 waiting을 false로 변경한다.
        /*
         * 임계구역: 알파벳 문자를 한 줄에 40개씩 10줄 출력한다.
         */
        for (int k = 0; k < 400; ++k) {
            printf("%s%c%s", color[i], 'A'+i, color[N]);
            if ((k+1) % 40 == 0)
                printf("\n");
        }
        /*
         * 임계구역에서 작업을 수행하고 임계구역을 벗어나기 전에 임계구역에 진입하기 위해
         * 기다리는 스레드가 있는지 확인한다. 기다리는 스레드가 있다면 헤당 스레드의 waiting만
         * false로 변경하여 lock을 넘겨주고 기다리는 스레드가 없다면 lock을 해제하고 임계구역을
         * 빠져나온다. 이를 통해서 스레드들의 유한 대기를 보장해준다.
         */

        j = (i+1) % N;
        // i+1부터 N-1번째 스레드까지 0번째부터 i-1번째 스레드까지 임계 구역 진입을 기다리는 스레드가 있는지 확인한다.
        while ((j != i) && !waiting[j])
            j = (j+1) % N;
        if (j == i) // j가 i라는 것은 현재 임계구역 진입을 기다리는 스레드가 없음을 의미한다.
            lock = 0; // 이 경우 lock을 해제하고 임계구역을 벗어난다.
        else // 임계구역 진입을 기다리는 스레드가 있으면 lock을 해당 스레드에게 넘겨준다.
            waiting[j] = false; // waiting을 false로 바꾸었기 때문에 스레드 j는 lock이 걸려있음에도 임계구역에 진입할 수 있다.
    }
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid[N];
    int i, id[N];

    /*
     * N 개의 자식 스레드를 생성한다.
     */
    for (i = 0; i < N; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, worker, id+i);
    }
    /*
     * 스레드가 출력하는 동안 RUNTIME 마이크로초 쉰다.
     * 이 시간으로 스레드의 출력량을 조절한다.
     */
    usleep(RUNTIME*4);
    /*
     * 스레드가 자연스럽게 무한 루프를 빠져나올 수 있게 한다.
     */
    alive = false;
    /*
     * 자식 스레드가 종료될 때까지 기다린다.
     */
    for (i = 0; i < N; ++i)
        pthread_join(tid[i], NULL);
    /*
     * 메인함수를 종료한다.
     */
    return 0;
}
