#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#define N 8
#define MAX 1024
#define BUFSIZE 4
#define RED "\e[0;31m"
#define RESET "\e[0m"

/* 생산자와 소비자가 공유할 버퍼를 만들고 필요한 변수를 초기화한다. */
int buffer[BUFSIZE];
int in = 0;
int out = 0;
int counter = 0;
int next_item = 0;

/* 생산된 아이템과 소비된 아이템의 로그와 개수를 기록하기 위한 변수 */
int task_log[MAX][2];
int produced = 0;
int consumed = 0;

/* alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다. */
bool alive = true;

/* 조건 변수와 뮤텍스 변수 */
pthread_mutex_t mutex;
pthread_cond_t cond_full;
pthread_cond_t cond_empty;

/* 생산자 스레드로 실행할 함수이다. 아이템을 생성하여 버퍼에 넣는다. */
void *producer(void *arg)
{
    int i = *(int *)arg;
    int item;
    
    while (true) {
        pthread_mutex_lock(&mutex);

        while (counter == BUFSIZE) {
            if (!alive) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&cond_full, &mutex);
        }

        if (!alive) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        item = next_item++;
        buffer[in] = item;
        in = (in + 1) % BUFSIZE;
        counter++;

        if (task_log[item][0] == -1) {
            task_log[item][0] = i;
            produced++;
        } else {
            printf("<P%d,%d>....ERROR: 아이템 %d 중복생산\n", i, item, item);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        pthread_cond_signal(&cond_empty);
        pthread_mutex_unlock(&mutex);

        printf("<P%d,%d>\n", i, item);
    }
}

/* 소비자 스레드로 실행할 함수이다. 버퍼에서 아이템을 읽고 출력한다. */
void *consumer(void *arg)
{
    int i = *(int *)arg;
    int item;

    while (true) {
        pthread_mutex_lock(&mutex);

        while (counter == 0) {
            if (!alive) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&cond_empty, &mutex);
        }

        if (!alive) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        item = buffer[out];
        out = (out + 1) % BUFSIZE;
        counter--;

        if (task_log[item][0] == -1) {
            printf(RED"<C%d,%d>"RESET"....ERROR: 아이템 %d 미생산\n", i, item, item);
            pthread_mutex_unlock(&mutex);
            continue;
        } else if (task_log[item][1] == -1) {
            task_log[item][1] = i;
            consumed++;
        } else {
            printf(RED"<C%d,%d>"RESET"....ERROR: 아이템 %d 중복소비\n", i, item, item);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        pthread_cond_signal(&cond_full);
        pthread_mutex_unlock(&mutex);

        printf(RED"<C%d,%d>"RESET"\n", i, item);
    }
}

int main(void)
{
    pthread_t tid[N];
    int i, id[N];

    /* 뮤텍스와 조건 변수 초기화 */
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_full, NULL);
    pthread_cond_init(&cond_empty, NULL);

    /* 생산자와 소비자를 기록하기 위한 logs 배열을 초기화한다. */
    for (i = 0; i < MAX; ++i)
        task_log[i][0] = task_log[i][1] = -1;

    /* N/2 개의 소비자 스레드를 생성한다. */
    for (i = 0; i < N/2; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, consumer, id+i);
    }

    /* N/2 개의 생산자 스레드를 생성한다. */
    for (i = N/2; i < N; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, producer, id+i);
    }

    /* 스레드가 출력하는 동안 1 밀리초 쉰다. */
    usleep(1000);

    /* 스레드가 자연스럽게 무한 루프를 빠져나올 수 있게 한다. */
    pthread_mutex_lock(&mutex);
    alive = false;
    pthread_cond_broadcast(&cond_full);  // 모든 생산자 스레드를 깨운다
    pthread_cond_broadcast(&cond_empty); // 모든 소비자 스레드를 깨운다
    pthread_mutex_unlock(&mutex);

    /* 자식 스레드가 종료될 때까지 기다린다. */
    for (i = 0; i < N; ++i)
        pthread_join(tid[i], NULL);

    /* 뮤텍스와 조건 변수 파괴 */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_full);
    pthread_cond_destroy(&cond_empty);

    /* 생산된 아이템을 건너뛰지 않고 소비했는지 검증한다. */
    for (i = 0; i < consumed; ++i)
        if (task_log[i][1] == -1) {
            printf("....ERROR: 아이템 %d 미소비\n", i);
            return 1;
        }

    /* 생산된 아이템의 개수와 소비된 아이템의 개수를 출력한다. */
    if (next_item == produced) {
        printf("Total %d items were produced.\n", produced);
        printf("Total %d items were consumed.\n", consumed);
    } else {
        printf("....ERROR: 생산량 불일치\n");
        return 1;
    }

    return 0;
}
