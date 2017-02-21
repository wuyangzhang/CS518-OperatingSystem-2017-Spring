/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2 
*/

#include <assert.h>
#include "my_pthread_t.h"

Node* createNode(my_pthread_t thread){
	Node* node = (Node*) malloc(sizeof(Node));
	node->thread = thread;
	return node;
}

void insertNode(Node* node){
	total_thread++;
	if(head == NULL){
		node->thread._self_id = 1;
		head = node;
		tail = node;
	}else{
		node->thread._self_id = tail->thread._self_id + 1;
		tail->next = node;
		node->prev = tail;
		tail = node;
	}
}

void removeNode(Node* node){

}


static inline my_pthread_t*
findThread_id(pid_t thread_id){
	curr = head;
	while(curr != NULL){
		if(curr->thread._self_id == thread_id){
			return &curr->thread;
		}
		curr = curr->next;
	}
	return NULL;
}

my_pthread_t*
findThread_robin(){
	if(scheduler.runningThread == total_thread){
		return findThread_id(1);
	}else{
		return findThread_id(scheduler.runningThread++);
	}
}

static inline my_pthread_t*
findThread_priority(){
	curr = head;
	int maxPriority = -1;
	pid_t priorityThread = 0;
	while(curr != NULL){
		if(curr->thread.priority > maxPriority){
			maxPriority = curr->thread.priority;
			curr->thread._self_id == priorityThread;
		}
		curr = curr->next;
	}
	return findThread_id(priorityThread);
}

int 
my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, 
					void*(*function)(void*),void* arg){
	assert(total_thread != MAX_THREADS);
	assert(getcontext(&thread->_ucontext_t) != -1);
	thread->func = function;
	thread->arg = arg;
	thread->_ucontext_t.uc_stack.ss_sp = thread->stack;
	thread->_ucontext_t.uc_stack.ss_size = MIN_STACK;
	thread->_ucontext_t.uc_stack.ss_flags = 0;
	thread->_ucontext_t.uc_flags = 0;
	thread->_ucontext_t.uc_link = NULL;
	thread->state = READY;

	makecontext(&(thread->_ucontext_t), function, 0);

	insertNode(createNode(*thread));

	return thread->_self_id;
}

void
my_pthread_yied(){

}

void
my_pthread_exit(void* value_ptr){
	free(((my_pthread_t*) value_ptr)->_ucontext_t.uc_stack.ss_sp);
}

int
my_pthread_join(my_pthread_t thread, void**value_ptr){
	return 0;
}

void
schedule(){

	if(total_thread > 1){
		printf("multiple thread\n\n\n\n\n");
		my_pthread_t* prevThread = findThread_id(scheduler.runningThread);
		my_pthread_t* currThread = findThread_robin();
		scheduler.runningThread = currThread->_self_id;
		swapcontext(&prevThread->_ucontext_t, &currThread->_ucontext_t);
	}
	
}

void
start(){

	signal(SIGALRM,schedule);
	struct itimerval tick;
	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 1;
	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = TIME_QUANTUM;
	assert(setitimer(ITIMER_REAL,&tick,NULL)!=1);
}

int
my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){
	return 0;
}

int
my_pthread_mutex_lock(my_pthread_mutex_t* mutex){
	return 0;
}

int 
my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){
	return 0;
}

int 
my_pthread_mutex_destory(my_pthread_mutex_t* mutex){
	return 0;
}

void* test(){
	while(1){
		printf("create thread1");
	}
	return NULL;
}

void* test2(){
	while(1){
		printf("create thread2");
	}
	return NULL;
}

int main(){
	start();
	my_pthread_t thread;
	my_pthread_t thread2;
	my_pthread_create(&thread,NULL,&test,NULL);
	my_pthread_create(&thread2,NULL,&test2,NULL);

	while(1){
		//printf("main");
	}

	return 0;
}
