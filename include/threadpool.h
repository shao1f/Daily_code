#pragma once

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

typedef struct worker {
    void* (*process) (void* arg);//执行的函数
    void* arg;//参数
    struct worker* next;
} Cthread_worker;

//线程池

typedef struct thread_pool {
    pthread_mutex_t queue_lock;//互斥锁
    pthread_cond_t queue_read;//条件变量
    Cthread_worker* queue_head;//工作队列
    int shutdown;//是否销毁线程池
    pthread_t* thread_id;
    int max_thread_num;//最大工作线程数
    int cur_queue_size;//当前等待线程数
} Cthread_pool;

static Cthread_pool* pool = NULL;

void pool_init(int max_thread_num);
void thread_destroy();
void* thread_run(void* arg);
void pool_add_work(void*(process) (void* arg),void* arg);
