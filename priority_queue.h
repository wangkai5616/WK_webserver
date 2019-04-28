#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdlib.h>

#define wk_PQ_DEFAULT_SIZE 10

typedef int (*wk_pq_comparator_pt)(void *pi, void *pj);

typedef struct priority_queue{//С����
    void **pq;//���ȶ��нڵ�ָ�룬ָ��ָ����Ǳ������ȼ����е�����
    size_t nalloc;//���ȶ���ʵ��Ԫ�ظ���
    size_t size;//���ȶ��д�С
    wk_pq_comparator_pt comp;//��ģʽ
}wk_pq_t;

int wk_pq_init(wk_pq_t *wk_pq, wk_pq_comparator_pt comp, size_t size);
int wk_pq_is_empty(wk_pq_t *wk_pq);
size_t wk_pq_size(wk_pq_t *wk_pq);
void *wk_pq_min(wk_pq_t *wk_pq);
int wk_pq_delmin(wk_pq_t *wk_pq);
int wk_pq_insert(wk_pq_t *wk_pq, void *item);
int wk_pq_sink(wk_pq_t *wk_pq, size_t i);

#endif 
