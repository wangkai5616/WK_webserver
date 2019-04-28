#include "threadpool.h"

static int threadpool_free(wk_threadpool_t *pool);
static void* threadpool_worker(void *arg);

// 释放线程池
int threadpool_free(wk_threadpool_t *pool){
    if(pool == NULL || pool->started > 0)
        return -1;

    // 释放线程数组
    if(pool->threads)
        free(pool->threads);
    

    // 逐节点销毁task链表
    wk_task_t *old;
    while(pool->head->next){
        old = pool->head->next;
        pool->head->next = pool->head->next->next;
        free(old);
    }
    return 0;
}

//工作线程
void *threadpool_worker(void *arg){
    if(arg == NULL)
        return NULL;
    

    wk_threadpool_t *pool = (wk_threadpool_t *)arg;
    wk_task_t *task;
    while(1){
        // 对线程池上锁
        pthread_mutex_lock(&(pool->lock));

        // 没有task且未停机则阻塞
        while((pool->queue_size == 0) && !(pool->shutdown))
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        
        // 立即停机模式、平滑停机且没有未完成任务则退出
        if(pool->shutdown == immediate_shutdown)
            break;
        else if((pool->shutdown == graceful_shutdown) && (pool->queue_size == 0))
            break;

        // 得到第一个task
        task = pool->head->next;
        // 没有task则开锁并进行下一次循环
        if(task == NULL){
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }

        // 存在task则取走并开锁
        pool->head->next = task->next;
        pool->queue_size--;
		//你先解开锁，然后执行这个task,放心，因为设置了参数EPOLLONESHOT，不会被其它线程处理
        pthread_mutex_unlock(&(pool->lock));

        // 设置task中func参数
        (*(task->func))(task->arg);
        free(task);
    }
    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
	//单个线程退出
    pthread_exit(NULL);
    return NULL;
}

// 回收线程资源
int threadpool_destory(wk_threadpool_t *pool, int graceful){
    if(pool == NULL)
        return wk_tp_invalid;
    if(pthread_mutex_lock(&(pool->lock)) != 0)
        return wk_tp_lock_fail;

    int err = 0;
    do{
        if(pool->shutdown){
            err = wk_tp_already_shutdown;
            break;
        }

        pool->shutdown = (graceful) ? graceful_shutdown : immediate_shutdown;

        if(pthread_cond_broadcast(&(pool->cond)) != 0){
            err = wk_tp_cond_broadcast;
            break;
        }

        if(pthread_mutex_unlock(&(pool->lock)) != 0){
            err = wk_tp_lock_fail;
            break;
        }

        // 回收每个线程资源
        for(int i = 0; i < pool->thread_count; i++){
            if(pthread_join(pool->threads[i], NULL) != 0){
                err = wk_tp_thread_fail;
            }
        }
    }while(0);

    if(!err){
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
        threadpool_free(pool);
    }
    return err;
}

// 初始化线程池
wk_threadpool_t *threadpool_init(int thread_num){
    // 分配线程池
    wk_threadpool_t* pool;
    if((pool = (wk_threadpool_t *)malloc(sizeof(wk_threadpool_t))) == NULL)
        goto err;

    // threads指针指向线程数组（存放tid），数组大小即为线程数
    pool->thread_count = 0;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->started = 0;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_num);
    
    // 分配并初始化task头结点
    pool->head = (wk_task_t*)malloc(sizeof(wk_task_t));
    if((pool->threads == NULL) || (pool->head == NULL))
        goto err;
    pool->head->func = NULL;
    pool->head->arg = NULL;
    pool->head->next = NULL;

    // 初始化锁
    if(pthread_mutex_init(&(pool->lock), NULL) != 0)
        goto err;

    // 初始化条件变量
    if(pthread_cond_init(&(pool->cond), NULL) != 0)
        goto err;

    // 创建线程
    for(int i = 0; i < thread_num; ++i){
        if(pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void*)pool) != 0){
            threadpool_destory(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }
    return pool;

err:
    if(pool)
        threadpool_free(pool);
    return NULL;
}

//添加任务
int threadpool_add(wk_threadpool_t* pool, void (*func)(void *), void *arg){
    int rc, err = 0;
    if(pool == NULL || func == NULL)
        return -1;

    if(pthread_mutex_lock(&(pool->lock)) != 0)
        return -1;

    // 已设置关机
    if(pool->shutdown){
        err = wk_tp_already_shutdown;
        goto out;
    }

    // 新建task并注册信息
    wk_task_t *task = (wk_task_t *)malloc(sizeof(wk_task_t));
    if(task == NULL)
        goto out;
    task->func = func;
    task->arg = arg;

    // 新task节点在head处插入
    task->next = pool->head->next;
    pool->head->next = task;
    pool->queue_size++;

	//通知有任务来了
    rc = pthread_cond_signal(&(pool->cond));

out:
    if(pthread_mutex_unlock(&pool->lock) != 0)
        return -1;
    return err;
}
