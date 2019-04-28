#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

typedef struct wk_task{
    void (*func)(void*);
    void* arg;
    struct wk_task* next;    // 任务链表（下一节点指针）
}wk_task_t;

typedef struct threadpool{
    pthread_mutex_t lock;    // 互斥锁
    pthread_cond_t cond;    // 条件变量
    pthread_t *threads;    // 线程
    wk_task_t *head;    // 任务链表
    int thread_count;    // 线程数 
    int queue_size;    // 任务链表长
    int shutdown;     // 关机模式
    int started;  //wk 应该是正在执行的线程个数
}wk_threadpool_t;

typedef enum{
    wk_tp_invalid = -1,
    wk_tp_lock_fail = -2,
    wk_tp_already_shutdown = -3,
    wk_tp_cond_broadcast = -4,
    wk_tp_thread_fail = -5,
}wk_threadpool_error_t;

typedef enum{
    immediate_shutdown = 1,//不管任务队列中是否还有任务待执行，立即关闭线程池
    graceful_shutdown = 2//等到执行完任务队列中的所有任务之后再关闭线程池
}wk_threadpool_sd_t;

wk_threadpool_t* threadpool_init(int thread_num);
int threadpool_add(wk_threadpool_t* pool, void (*func)(void *), void* arg);
int threadpool_destroy(wk_threadpool_t* pool, int gracegul);

#endif
