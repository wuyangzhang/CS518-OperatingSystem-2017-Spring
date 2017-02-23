/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2 
*/
           
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>

#ifndef _TEST_H_
#define _TEST_H_

#define MAX_THREADS 1024
#define MIN_STACK 32768
#define TIME_QUANTUM 50000
#define QUEUELEVEL 4

typedef int pid_t;

enum THREAD_STATE{
  READY = 0,
  RUNNING,
  BLOCKED,
  WAITING,
  TERMINATED
};

typedef struct _my_pthread_t{
    pid_t _self_id;
    ucontext_t _ucontext_t;
    char stack[MIN_STACK];
    void* (*func)(void *arg);
    void* arg;
    int state;
    int priority;
}my_pthread_t;

typedef struct _my_pthread_mutex_t{
    int flag;
}my_pthread_mutex_t;

/*
  schedulercontexts
  */
typedef struct _Node
{
  my_pthread_t* thread;
  struct _Node* next;
  //struct _Node* prev;
}Node;

typedef struct{
  Node* head;
  Node* tail;
  int size;
} Queue;

typedef struct _schedule_t{
    my_pthread_t* runningThread;
    my_pthread_t schedule_thread; // main thread used to schedule
    pid_t runningThread_id;
    int total_thread;
    int total_pendingThread;
    char stack[MIN_STACK];
    int id;
}schedule_t;

int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void* arg);

void my_pthread_yied();

void pthread_exit(void* value_ptr);

int my_pthread_join(my_pthread_t thread, void**value_ptr);

int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);

int my_pthread_mutex_lock(my_pthread_mutex_t* mutex);

int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex);

int my_pthread_mutex_destory(my_pthread_mutex_t* mutex);


#endif