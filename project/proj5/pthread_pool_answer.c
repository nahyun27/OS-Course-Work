#include <stdlib.h>
#include <stdio.h>
#include "pthread_pool.h"

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param) {
    pthread_pool_t *pool = (pthread_pool_t *)param;
    task_t task;

    while (true) {
        pthread_mutex_lock(&pool->mutex);

        while (pool->state == ON && pool->q_len == 0)
            pthread_cond_wait(&pool->full, &pool->mutex);

        if (pool->state == OFF || (pool->state == STANDBY && pool->q_len == 0)) {            pthread_mutex_unlock(&pool->mutex);
            break;
        }
				
				task = pool->q[pool->q_front];
				pool->q[pool->q_front].function = NULL;
				pool->q[pool->q_front].param = NULL;
				pool->q_front = (pool->q_front + 1) % pool->q_size;
				pool->q_len--;
				
				pthread_cond_signal(&pool->empty);
        pthread_mutex_unlock(&pool->mutex);
        
        if (task.function != NULL)
            task.function(task.param);
    
		}
    return NULL;
}

/*
 * 스레드풀을 생성한다. bee_size는 일꾼(일벌) 스레드의 개수이고, queue_size는 대기열의 용량이다.
 * bee_size는 POOL_MAXBSIZE를, queue_size는 POOL_MAXQSIZE를 넘을 수 없다.
 * 일꾼 스레드와 대기열에 필요한 공간을 할당하고 변수를 초기화한다.
 * 일꾼 스레드의 동기화를 위해 사용할 상호배타 락과 조건변수도 초기화한다.
 * 마지막 단계에서는 일꾼 스레드를 생성하여 각 스레드가 worker() 함수를 실행하게 한다.
 * 대기열로 사용할 원형 버퍼의 용량이 일꾼 스레드의 수보다 작으면 효율을 극대화할 수 없다.
 * 이런 경우 사용자가 요청한 queue_size를 bee_size로 상향 조정한다.
 * 성공하면 POOL_SUCCESS를, 실패하면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size) {
    if (bee_size > POOL_MAXBSIZE || queue_size > POOL_MAXQSIZE)
        return POOL_FAIL;

    pool->state = ON;
    pool->q_front = 0;
    pool->q_len = 0;
    pool->bee_size = bee_size;
    pool->q_size = queue_size < bee_size ? bee_size : queue_size;


    pool->q = (task_t *)calloc(pool->q_size, sizeof(task_t));
    if (pool->q == NULL) {
        return POOL_FAIL;
    }

    pool->bee = (pthread_t *)calloc(pool->bee_size, sizeof(pthread_t));
    if (pool->bee == NULL) {
        free(pool->q);
        return POOL_FAIL;
    }

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->full, NULL);
    pthread_cond_init(&pool->empty, NULL);

    for (int i = 0; i < pool->bee_size; i++) {
        if (pthread_create(&pool->bee[i], NULL, worker, pool) != 0) {
            pool->state = OFF;
						for (int j = 0; j < i; j++) {
								pthread_cancel(pool->bee[j]); // 스레드 취소 요청
								pthread_join(pool->bee[j], NULL); // 조인하여 스레드 종료 대기
						}
						pthread_mutex_destroy(&pool->mutex);
            pthread_cond_destroy(&pool->full);
            pthread_cond_destroy(&pool->empty);
            free(pool->q);
            free(pool->bee);
            return POOL_FAIL;
        }
    }

    return POOL_SUCCESS;
}

/*
 * 스레드풀에서 실행시킬 함수와 인자의 주소를 넘겨주며 작업을 요청한다.
 * 스레드풀의 대기열이 꽉 찬 상황에서 flag이 POOL_NOWAIT이면 즉시 POOL_FULL을 리턴한다.
 * POOL_WAIT이면 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나온다.
 * 작업 요청이 성공하면 POOL_SUCCESS를 그렇지 않으면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag) {
    pthread_mutex_lock(&pool->mutex);

		if (pool->state == ON && flag == POOL_NOWAIT && pool->q_len == pool->q_size) {
        pthread_mutex_unlock(&(pool->mutex));
        return POOL_FULL;
    }

		while (pool->state == ON && flag == POOL_WAIT && pool->q_len == pool->q_size)
				pthread_cond_wait(&pool->empty, &pool->mutex);

		if ( pool->state == ON && pool->q_len < pool->q_size) {
				int next = (pool->q_front + pool->q_len) % pool->q_size;
				pool->q[next].function = f;
				pool->q[next].param = p; 
				pool->q_len++;
				pthread_cond_signal(&pool->full);
				pthread_mutex_unlock(&pool->mutex);
				return POOL_SUCCESS;
		}

    pthread_mutex_unlock(&pool->mutex);
    return POOL_FAIL;
}

/*
 * 스레드풀을 종료한다. 일꾼 스레드가 현재 작업 중이면 그 작업을 마치게 한다.
 * how의 값이 POOL_COMPLETE이면 대기열에 남아 있는 모든 작업을 마치고 종료한다.
 * POOL_DISCARD이면 대기열에 새 작업이 남아 있어도 더 이상 수행하지 않고 종료한다.
 * 메인(부모) 스레드는 종료된 일꾼 스레드와 조인한 후에 스레드풀에 할당된 자원을 반납한다.
 * 스레드를 종료시키기 위해 철회를 생각할 수 있으나 바람직하지 않다.
 * 락을 소유한 스레드를 중간에 철회하면 교착상태가 발생하기 쉽기 때문이다.
 * 종료가 완료되면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_shutdown(pthread_pool_t *pool, int how) {
    pthread_mutex_lock(&pool->mutex);

    if (how == POOL_COMPLETE){
        pool->state = STANDBY;
				while (pool->q_len > 0)
						pthread_cond_wait(&pool->empty, &pool->mutex); 
		}
    else if (how == POOL_DISCARD)
        pool->state = OFF;

    pthread_cond_broadcast(&pool->full);
    pthread_cond_broadcast(&pool->empty);

    pool->state = OFF;
    pthread_mutex_unlock(&pool->mutex); 

    for (int i = 0; i < pool->bee_size; i++)
        pthread_join(pool->bee[i], NULL);

    free(pool->bee);
    free(pool->q);
    pool->bee = NULL;
    pool->q = NULL;
		
    return POOL_SUCCESS;
}
