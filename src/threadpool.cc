#include "../include/threadpool.h"

void pool_init(int max_thread_num){
    pool = (Cthread_pool*)malloc(sizeof(Cthread_pool));
    pthread_mutex_init(&(pool->queue_lock),NULL);
    pthread_cond_init(&(pool->queue_read),NULL);
    pool->max_thread_num = max_thread_num;
    pool->queue_head = NULL;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;
    pool->thread_id = (pthread_t*)malloc(max_thread_num*sizeof(pthread_t));
    
    int i = 0;
    for(;i<max_thread_num;++i){
        pthread_create(&(pool->thread_id[i]),NULL,thread_run,NULL);
    }

    return;
}

void thread_destroy(){
    if(pool->shutdown){
        //防止二次释放
        return;
    }

    pool->shutdown = 1;
    //唤醒所有等待线程
    pthread_cond_broadcast(&(pool->queue_read));

    int i = 0;
    for(;i<pool->max_thread_num;++i){
        pthread_join(pool->thread_id[i],NULL);
    }

    free(pool->thread_id);

    Cthread_worker* head = NULL;
    while(pool->queue_head != NULL){
        head = pool->queue_head;
        pool->queue_head = head->next;
        free(head);
    }

    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_read));

    free(pool);
    pool = NULL;

    return;
}

void pool_add_work(void* (*process) (void* arg),void* arg){
    Cthread_worker* new_work = (Cthread_worker*)malloc(sizeof(Cthread_worker));
    new_work->process = process;
    new_work->arg = arg;
    new_work->next = NULL;

    //要操作任务链表,需要加锁
    pthread_mutex_lock(&(pool->queue_lock));

    Cthread_worker* cur = pool->queue_head;

    if(cur != NULL){
        while(cur->next != NULL){
            cur = cur->next;
        }
        cur->next = new_work;//将新任务插入链表
    }else {
        pool->queue_head = new_work;
    }

    assert(pool->queue_head != NULL);
    pool->cur_queue_size++;//同步修改等待队列的长度
    pthread_mutex_unlock(&(pool->queue_lock));
    //等待任务队列插入了新任务,唤醒睡眠线程去处理,如果没有睡眠线程,则不处理
    pthread_cond_signal(&(pool->queue_read));
}

void* thread_run(void* arg){
    (void)arg;
    while(1){
        pthread_mutex_lock(&(pool->queue_lock));

        while(pool->cur_queue_size == 0 && !pool->shutdown){
            //pthread_cond_wait是原子操作,在等待前释放锁,等待成功加锁
            pthread_cond_wait(&(pool->queue_read),&(pool->queue_lock));
        }

        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit(NULL);
        }

        assert(pool->cur_queue_size != 0);
        assert(pool->queue_head != NULL);

        Cthread_worker* head = pool->queue_head;
        pool->queue_head = head->next;
        pool->cur_queue_size--;
        //处理完任务链表,解锁,让其他线程继续处理任务
        pthread_mutex_unlock(&pool->queue_lock);
        (*(head->process))(head->arg);
        free(head);
        head = NULL;
    }
}
