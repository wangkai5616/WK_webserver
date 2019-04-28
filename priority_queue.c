#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"

//交换优先级队列中的两个元素
void exch(wk_pq_t *wk_pq, size_t i, size_t j){
    void *tmp = wk_pq->pq[i];
    wk_pq->pq[i] = wk_pq->pq[j];
    wk_pq->pq[j] = tmp;
}

//将下标为k的元素进行上移（新插入的元素当然要上移）
void swim(wk_pq_t *wk_pq, size_t k){
    while (k > 1 && wk_pq->comp(wk_pq->pq[k], wk_pq->pq[k/2])){
        exch(wk_pq, k, k/2);
        k /= 2;
    }
}

//将下标为k的元素进行下沉
int sink(wk_pq_t *wk_pq, size_t k){
    size_t j;
    size_t nalloc = wk_pq->nalloc;
    while((k << 1) <= nalloc){
        j = k << 1;
        if((j < nalloc) && (wk_pq->comp(wk_pq->pq[j+1], wk_pq->pq[j])))
            j++;

        if(!wk_pq->comp(wk_pq->pq[j], wk_pq->pq[k]))
            break;

        exch(wk_pq, j, k);
        k = j;
    }
    return k;
}

int wk_pq_sink(wk_pq_t *wk_pq, size_t i){
    return sink(wk_pq, i);
}

//初始化优先级队列
int wk_pq_init(wk_pq_t *wk_pq, wk_pq_comparator_pt comp, size_t size){
    // 为wk_pq_t节点的pq分配(void *)指针
    wk_pq->pq = (void **)malloc(sizeof(void *) * (size + 1));
    if (!wk_pq->pq)
        return -1;

    wk_pq->nalloc = 0;
    wk_pq->size = size + 1;
    wk_pq->comp = comp;
    return 0;
}

int wk_pq_is_empty(wk_pq_t *wk_pq){
    // 通过nalloc值款快速判断是否为空
    return (wk_pq->nalloc == 0) ? 1 : 0;
}

size_t wk_pq_size(wk_pq_t *wk_pq){
    // 获取优先队列大小
    return wk_pq->nalloc;
}

//优先级队列的最小值
void *wk_pq_min(wk_pq_t *wk_pq){
    // 优先队列最小值直接返回第一个元素（指针）
    if (wk_pq_is_empty(wk_pq))
        return (void *)(-1);

    return wk_pq->pq[1];
}

//修改优先级队列大小
int resize(wk_pq_t *wk_pq, size_t new_size){
    if(new_size <= wk_pq->nalloc)
        return -1;

    void **new_ptr = (void **)malloc(sizeof(void *) * new_size);
    if(!new_ptr)
        return -1;
    // 将原本nalloc + 1个元素值拷贝到new_ptr指向的位置
    memcpy(new_ptr, wk_pq->pq, sizeof(void *) * (wk_pq->nalloc + 1));
    // 释放旧元素
    free(wk_pq->pq);
    // 重新改写优先队列元素pq指针为new_ptr
    wk_pq->pq = new_ptr;
    wk_pq->size = new_size;
    return 0;
}

//删除优先级队列最小元素，其实就是删除堆顶元素
int wk_pq_delmin(wk_pq_t *wk_pq){
    if(wk_pq_is_empty(wk_pq))
        return 0;

    exch(wk_pq, 1, wk_pq->nalloc);
    --wk_pq->nalloc;
    sink(wk_pq, 1);
    if((wk_pq->nalloc > 0) && (wk_pq->nalloc <= (wk_pq->size - 1)/4)){
        if(resize(wk_pq, wk_pq->size / 2) < 0)
            return -1;
    }
    return 0;
}

//向优先级队列插入一个元素
//如果优先级队列的大小不足，那么是以2倍的方式扩容
int wk_pq_insert(wk_pq_t *wk_pq, void *item){
    if(wk_pq->nalloc + 1 == wk_pq->size){
        if(resize(wk_pq, wk_pq->size * 2) < 0){
            return -1;
        }
    }
    wk_pq->pq[++wk_pq->nalloc] = item;
    swim(wk_pq, wk_pq->nalloc);
    return 0;
}



