#ifndef wk_TIMER_H
#define wk_TIMER_H

#include "priority_queue.h"
#include "http_request.h"

#define TIMEOUT_DEFAULT 500     /* ms */

// 函数指针，负责超时处理，wk_add_timer时指定处理函数
typedef int (*timer_handler_pt)(wk_http_request_t* request);

//时间结构体
typedef struct wk_timer{
    size_t key;    // 标记超时时间
    int deleted;    // 标记是否被删除
    timer_handler_pt handler;    // 超时处理，add时指定
    wk_http_request_t* request;    // 指向对应的request请求
} wk_timer_t;

// wk_pq_t定义在"priority_queue.h"中，优先队列中节点
extern wk_pq_t wk_timer;
extern size_t wk_current_msec;

int wk_timer_init();
int wk_find_timer();
void wk_handle_expire_timers();
void wk_add_timer(wk_http_request_t* request, size_t timeout, timer_handler_pt handler);
void wk_del_timer(wk_http_request_t* request);
int timer_comp(void *ti, void *tj);

#endif