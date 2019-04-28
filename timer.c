#include <sys/time.h>
#include "timer.h"

wk_pq_t wk_timer;
//全局的时间
size_t wk_current_msec;

//按照超时时间进行比较
int timer_comp(void* ti, void* tj){
    wk_timer_t* timeri = (wk_timer_t*)ti;
    wk_timer_t* timerj = (wk_timer_t*)tj;
    return (timeri->key < timerj->key) ? 1 : 0;
}

void wk_time_update(){
    // 获取当前时间
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    wk_current_msec = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

//初始化时间结构
int wk_timer_init(){
    // 建立连接后立即初始化
    // 初始优先队列大小wk_PQ_DEFAULT_SIZE = 10
    int rc = wk_pq_init(&wk_timer, timer_comp, wk_PQ_DEFAULT_SIZE);

    // 更新当前时间
    wk_time_update();
    return 0;
}

int wk_find_timer(){
    int time;
    // 返回队列中最早时间和当前时间之差
    while(!wk_pq_is_empty(&wk_timer)){
        // 更新当前时间
        wk_time_update();
        // timer_node指向最小的时间
        wk_timer_t* timer_node = (wk_timer_t*)wk_pq_min(&wk_timer);
        // 如果已删则释放此节点（wk_del_timer只置位不删除）
        if(timer_node->deleted){
            int rc = wk_pq_delmin(&wk_timer);
            free(timer_node);
            continue;
        }
        // 此时timer_node为时间最小节点，time为优先队列里最小时间减去当前时间
        time = (int)(timer_node->key - wk_current_msec);
        time = (time > 0) ? time : 0;
        break;
    }
    return time;
}

//处理超时
void wk_handle_expire_timers(){
    while(!wk_pq_is_empty(&wk_timer)){
        // 更新当前时间
        wk_time_update();
        wk_timer_t* timer_node = (wk_timer_t*)wk_pq_min(&wk_timer);
        // 如果已删则释放此节点
        if(timer_node->deleted){
            int rc = wk_pq_delmin(&wk_timer); 
            free(timer_node);
            continue;
        }
        // 最早入队列节点超时时间大于当前时间（未超时）
        // 结束超时检查，顺带删了下标记为删除的节点
        if(timer_node->key > wk_current_msec){
            return;
        }
        // 出现了没被删但是超时的情况，调用handler处理
        if(timer_node->handler){
            timer_node->handler(timer_node->request);
        }
        int rc = wk_pq_delmin(&wk_timer); 
        free(timer_node);
    }
}

//新增时间戳
//连接是有时间限制的，所以需要进行管理，到了超时时间之后，连接要关闭
void wk_add_timer(wk_http_request_t* request, size_t timeout, timer_handler_pt handler){
    wk_time_update();
    // 申请新的wk_timer_t节点，并加入到wk_http_request_t的timer下
    wk_timer_t* timer_node = (wk_timer_t*)malloc(sizeof(wk_timer_t));
    request->timer = timer_node;
    // 加入时设置超时阈值，删除信息等
    timer_node->key = wk_current_msec + timeout;
    timer_node->deleted = 0;
    timer_node->handler = handler;
    // 注意需要在wk_timer_t节点中反向设置指向对应resquest的指针
    timer_node->request = request;
    // 将新节点插入优先队列
    int rc = wk_pq_insert(&wk_timer, timer_node);
}

//删除时间戳
void wk_del_timer(wk_http_request_t* request) {
    wk_time_update();
    wk_timer_t* timer_node = request->timer;
    // 惰性删除
    // 标记为已删，在find_timer和handle_expire_timers检查队列时会删除
    timer_node->deleted = 1;
}