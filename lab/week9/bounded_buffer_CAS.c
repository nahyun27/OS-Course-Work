/*
 * Copyright(c) 2021-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 학과: 컴퓨터학부
 * 학번: 2019055014
 * 이름: 김민수
 * 수정 내용: 공유 버퍼 동기화 구현 및 usleep 시간 수정
 * 수정 날짜: 2024.05.02
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 8
#define MAX 10240
#define BUFSIZE 4
#define RUNTIME 1000    /* 출력량을 제한하기 위한 실행시간 (마이크로초) */
#define RED "\e[0;31m"
#define RESET "\e[0m"

/*
 * lock은 spinlock 구현을 위해 사용할 변수이다.
 */
atomic_int lock = 0;
/*
 * 생산자와 소비자가 공유할 버퍼를 만들고 필요한 변수를 초기화한다.
 */
int buffer[BUFSIZE];
int in = 0;
int out = 0;
int counter = 0;
int next_item = 0;
/*
 * 생산된 아이템과 소비된 아이템의 로그와 개수를 기록하기 위한 변수
 */
int task_log[MAX][2];
int produced = 0;
int consumed = 0;
/*
 * alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다.
 */
bool alive = true;

/*
 * 생산자 스레드로 실행할 함수이다. 아이템을 생성하여 버퍼에 넣는다.
 */
void *producer(void *arg)
{
    int i = *(int *)arg;
    int item;
    
    while (alive) {
        int expected = 0;
        /*
         * spinlock을 통해 공유 버퍼에 대한 동기화가 이루어 질 수 있도록 한다.
         * lock이 0이면 1로 바꾸고 true를 반환하여 루프를 빠져나오게 되지만
         * 그렇지 않은 경우는 expected가 1로 바뀌기 때문에 다시 0으로 바꾼다.
         * 또한 false를 반환하기 때문에 루프에 머무르며 대기하도록 한다.
         *
         */
        while(!atomic_compare_exchange_weak(&lock, &expected, 1))
            expected = 0;

        /**
         * 임계구역에 진입하였다면 producer가 버퍼에 아이템을 넣을 수 있는지 확인한다.
         * 만약 버퍼가 가득 찼다면 lock을 해제하고 임계 구역을 빠져나오고 그렇지 않다면
         * 아이템을 생산하고 버퍼에 넣는다.
         */
        if (counter == BUFSIZE) {
            lock = 0;
            continue;
        }

        /*
         * 새로운 아이템을 생산하여 버퍼에 넣고 관련 변수를 갱신한다.
         */
        item = next_item++;
        buffer[in] = item;
        in = (in + 1) % BUFSIZE;
        counter++;
        /*
         * 생산자를 기록하고 중복생산이 아닌지 검증한다.
         */
        if (task_log[item][0] == -1) {
            task_log[item][0] = i;
            produced++;
        }
        else {
            printf("<P%d,%d>....ERROR: 아이템 %d 중복생산\n", i, item, item);
            lock = 0; // 중복생산이 발생하는 경우 데드락에 걸리지 않도록 lock을 해제한다.
            continue;
        }
        //위의 작업을 다 마치고 나면 lock을 해제하고 임계구역을 빠져나온다.
        lock = 0;
        /*
         * 생산한 아이템을 출력한다.
         */
        printf("<P%d,%d>\n", i, item);
    }
    pthread_exit(NULL);
}

/*
 * 소비자 스레드로 실행할 함수이다. 버퍼에서 아이템을 읽고 출력한다.
 */
void *consumer(void *arg)
{
    int i = *(int *)arg;
    int item;
    
    while (alive) {
        int expected = 0;
        /*
         * producer 함수와 마찬가지로 spinlock을 통해 공유 버퍼에 대한 동기화가
         * 이루어 질 수 있도록 한다.
         */
        while(!atomic_compare_exchange_weak(&lock, &expected, 1))
            expected = 0;

        /*
         * 임계구역에 진입하였다면 consumer가 소비할 수 있는 아이템이 있는지 확인한다.
         * 만약 버퍼가 비어 있다면 lock을 해제하고 임계 구역을 빠져나오고 그렇지 않다면
         * 버퍼에 있는 아이템을 소비한다.
         */
        if (counter == 0) {
            lock = 0;
            continue;
        }
        /*
         * 버퍼에서 아이템을 꺼내고 관련 변수를 갱신한다.
         */
        item = buffer[out];
        out = (out + 1) % BUFSIZE;
        counter--;
        /*
         * 소비자를 기록하고 미생산 또는 중복소비 아닌지 검증한다.
         */
        if (task_log[item][0] == -1) {
            printf(RED"<C%d,%d>"RESET"....ERROR: 아이템 %d 미생산\n", i, item, item);
            lock = 0; // 아이템 미생산이 발생하는 경우 데드락에 걸리지 않도록 lock을 해제한다.
            continue;
        }
        else if (task_log[item][1] == -1) {
            task_log[item][1] = i;
            consumed++;
        }
        else {
            printf(RED"<C%d,%d>"RESET"....ERROR: 아이템 %d 중복소비\n", i, item, item);
            lock = 0; // 중복소비가 발생하는 경우 데드락에 걸리지 않도록 lock을 해제한다.
            continue;
        }
        //위의 작업을 다 마치고 나면 lock을 해제하고 임계구역을 빠져나온다.
        lock = 0;
        /*
         * 소비할 아이템을 빨간색으로 출력한다.
         */
        printf(RED"<C%d,%d>"RESET"\n", i, item);
    }
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid[N];
    int i, id[N];

    /*
     * 생산자와 소비자를 기록하기 위한 logs 배열을 초기화한다.
     */
    for (i = 0; i < MAX; ++i)
        task_log[i][0] = task_log[i][1] = -1;
    /*
     * N/2 개의 소비자 스레드를 생성한다.
     */
    for (i = 0; i < N/2; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, consumer, id+i);
    }
    /*
     * N/2 개의 생산자 스레드를 생성한다.
     */
    for (i = N/2; i < N; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, producer, id+i);
    }
    /*
     * 스레드가 출력하는 동안 RUNTIME 마이크로초 쉰다.
     * 이 시간으로 스레드의 출력량을 조절한다.
     */
    usleep(RUNTIME*100); //공유 버퍼 생산 소비가 50이 넘도록 시간을 늘렸음.
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
     * 생산된 아이템을 건너뛰지 않고 소비했는지 검증한다.
     */
    for (i = 0; i < consumed; ++i)
        if (task_log[i][1] == -1) {
            printf("....ERROR: 아이템 %d 미소비\n", i);
            return 1;
        }
    /*
     * 생산된 아이템의 개수와 소비된 아이템의 개수를 출력한다.
     */
    if (next_item == produced) {
        printf("Total %d items were produced.\n", produced);
        printf("Total %d items were consumed.\n", consumed);
    }
    else {
        printf("....ERROR: 생산량 불일치\n");
        return 1;
    }
    /*
     * 메인함수를 종료한다.
     */
    return 0;
}
