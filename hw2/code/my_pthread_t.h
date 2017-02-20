/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2 
	hint: 
	(1) investigate system library ucontext.h
	(2) interrupt context, set an interruption time in 50ms. Implement a signal handler in which we schedule and swap
	(3) multi-level priority queue
*/

/* 
Userlevel context

typedef struct ucontext
  {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    __sigset_t uc_sigmask;
    mcontext_t uc_mcontext;
  } ucontext_t;

  typedef struct {
               void  *ss_sp;     // Base address of stack 
               int    ss_flags;  // Flags 
               size_t ss_size;   // Number of bytes in stack 
           } stack_t;

*/
           
#include <stdlib.h>
#include <ucontext.h>
#include < sys/time.h>

#ifndef _MY_PTHREAD_T_H_
#define _MY_PTHREAD_T_H_

#define MAX_THREADS 1024
#define MIN_STACK 32768
#define TIME_QUANTUM 50

typedef struct _pthread_t{

}pthread_t

int my_pthread_create(pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void* arg);

void my_pthread_yied();

void pthread_exit(void* value_ptr);

int my_pthread_join(pthread_t thread, void**value_ptr);

int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);

int my_pthread_mutex_lock(my_pthread_mutex_t* mutex);

int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex);

int my_pthread_mutex_destory(my_pthread_mutex_t* mutex);

#endif