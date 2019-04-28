#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdlib.h>

#define wk_PQ_DEFAULT_SIZE 10

typedef int (*wk_pq_comparator_pt)(void *pi, void *pj);

typedef struct priority_queue{//小根堆
    void **pq;//优先队列节点指针，指针指向的是保存优先级队列的数组
    size_t nalloc;//优先队列实际元素个数
    size_t size;//优先队列大小
    wk_pq_comparator_pt comp;//堆模式
}wk_pq_t;

int wk_pq_init(wk_pq_t *wk_pq, wk_pq_comparator_pt comp, size_t size);
int wk_pq_is_empty(wk_pq_t *wk_pq);
size_t wk_pq_size(wk_pq_t *wk_pq);
void *wk_pq_min(wk_pq_t *wk_pq);
int wk_pq_delmin(wk_pq_t *wk_pq);
int wk_pq_insert(wk_pq_t *wk_pq, void *item);
int wk_pq_sink(wk_pq_t *wk_pq, size_t i);

#endif 
