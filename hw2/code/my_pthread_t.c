/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2 
*/

#include <assert.h>

#include "my_pthread_t.h"

static int counter = 0;
static my_pthread_mutex_t countLock;

Node* createNode(my_pthread_t thread){
	Node* node = (Node*) malloc(sizeof(Node));
	node->thread = thread;
	return node;
}

void insertNode(Node* node){
	total_thread++;
	if(head == NULL){
		node->thread._self_id = 0;
		head = node;
		head->prev = head;
		head->next = head;
	}else{
		node->thread._self_id = head->prev->thread._self_id + 1;
		node->prev = head->prev;
		node->next = head;
		head->prev->next = node;
		head->prev = node;
	}
}

void removeNode(pid_t thread_id){
	total_thread--;
	curr = head;
	while(curr != NULL){
		if(curr->thread._self_id == thread_id){
			free(curr->thread._ucontext_t.uc_stack.ss_sp);
			curr->prev->next = curr->next;
			curr->next->prev = curr->prev;
			free(curr);
		}
	}
}


static inline my_pthread_t*
findThread_id(pid_t thread_id){
		curr = head;
	while(curr != NULL){ //TODO:stuck into while loop with double linked list
		if(curr->thread._self_id == thread_id){
			return &curr->thread;
		}
		curr = curr->next;
	}
	return NULL;
}

my_pthread_t*
findThread_robin(){
	if(scheduler.runningThread == total_thread - 1){
		return &head->thread;
	}else{
		return findThread_id(++scheduler.runningThread);
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
	thread->_ucontext_t.uc_link = &head->thread._ucontext_t;
	thread->state = READY;

	makecontext(&(thread->_ucontext_t), function, 0);

	insertNode(createNode(*thread));

	return thread->_self_id;
}

void
my_pthread_yied(){

	if(scheduler.runningThread != -1){
		my_pthread_t* thread = findThread_id(scheduler.runningThread);
		thread->state = READY;
		//switch to main thread
		scheduler.runningThread = 0;
		swapcontext(&thread->_ucontext_t, &head->thread._ucontext_t);
	}
}

void
my_pthread_exit(void* value_ptr){
	pid_t thread_id = *((pid_t *) value_ptr);
	removeNode(thread_id);
}

int
my_pthread_join(my_pthread_t thread, void**value_ptr){
	return 0;
}

void
schedule(){

	if(total_thread > 1){
		my_pthread_t* prevThread = findThread_id(scheduler.runningThread);
		my_pthread_t* currThread = findThread_robin();
		scheduler.runningThread = currThread->_self_id;
		prevThread->state = READY;
		currThread->state = RUNNING;
		//printf("switch from thread %d to thread %d\n", prevThread->_self_id, currThread->_self_id);
		
		// if(prevThread->_self_id == 0){
			// setcontext(&currThread->_ucontext_t);
		// }else{
			assert(swapcontext(&prevThread->_ucontext_t, &currThread->_ucontext_t) != -1);
		// }
	}
}

void
start(){

	my_pthread_t t;
	my_pthread_create(&t,NULL,NULL,NULL);
	assert(getcontext(&t._ucontext_t) != -1);

	my_pthread_mutex_init(&countLock, NULL);

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
	mutex->flag = 0;
	return 0;
}

int
my_pthread_mutex_lock(my_pthread_mutex_t* mutex){
	while(__sync_lock_test_and_set(&mutex->flag,1))
		//my_pthread_yied();
	return 0;
}

int 
my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){
	__sync_lock_release(&mutex->flag);
	return 0;
}

int 
my_pthread_mutex_destory(my_pthread_mutex_t* mutex){
	return 0;
}

void* test(){
	printf("thread %d is running\n", scheduler.runningThread);
	int i;
	for(i = 0; i < 10000; i++){
		my_pthread_mutex_lock(&countLock);
		counter++;
		my_pthread_mutex_unlock(&countLock);
	}
	return NULL;
}

void* test2(){
	while(1){
		printf("22222\n");
	}
	return NULL;
}

void* test3(){
	while(1){
		printf("33333\n");
	}
	return NULL;
}

int main(){
	start();

	my_pthread_t thread;
	my_pthread_t thread2;
	my_pthread_t thread3;

	my_pthread_create(&thread,NULL,&test,NULL);
	my_pthread_create(&thread2,NULL,&test,NULL);
	my_pthread_create(&thread3,NULL,&test,NULL);

	while(1){
		printf("counter %d\n", counter);
	}

	printf("exit program\n");
	return 0;
}
