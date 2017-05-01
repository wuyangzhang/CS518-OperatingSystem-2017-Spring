/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2 
*/

#include <assert.h>
#include <string.h>
#include "test.h"

static schedule_t scheduler;
static int counter = 0;
static my_pthread_mutex_t countLock;
static int CONTEXT_INTERRUPTION = 0;
static Queue* pendingThreadQueue[QUEUELEVEL];
static Queue* finishedThreadQueue;

/*
*	Create a node for a thread
*/

void
queue_init(Queue* queue){
	queue->head = queue->tail = (Node*)malloc(sizeof(Node));
}

void
multiQueue_init(Queue* queue[QUEUELEVEL]){
	int i;
	for(i = 0; i < QUEUELEVEL; i++){
		queue_init(queue[i]);
	}
}
/*
*	Check queue is empty?
*	Return: empty -> 1, NOT empty -> 0;
*/
int
queue_empty(Queue* queue){
	if(queue->head->next == NULL){
		return 1;
	}else{
		return 0;
	}
}

/*
*	Check multiple priority queue is empty?
*	Return: empty -> 1, NOT empty -> 0;
*/
int
multiQueue_empty(Queue* queue[QUEUELEVEL]){
	int i;
	for(i = QUEUELEVEL - 1; i >= 0; i--){
		if(!queue_empty(queue[i]))
			return 0;
	}
	return 1;
}

void
multiQueue_push(Queue* queue[QUEUELEVEL], my_pthread_t* thread) {
	Node* node = (Node*)malloc(sizeof(Node));
	node->thread = thread;
	node->next = NULL;

	queue[thread->priority]->tail->next = node;
	queue[thread->priority]->tail = node;
	
	queue[thread->priority]->size++;
	scheduler.total_pendingThread++;
}

void
queue_push(Queue* queue, my_pthread_t* thread){
	Node* node = (Node*)malloc(sizeof(Node));
	node->thread = thread;
	node->next = NULL;

	queue->tail->next = node;
	queue->tail = node;
	
	queue->size++;
	scheduler.total_pendingThread++;
}

my_pthread_t* 
queue_pop(Queue* queue) {
	Node* node = NULL;

	if(!queue_empty(queue)){
		node = queue->head->next;
		queue->head->next = node->next;
		if(queue->tail == node){
			queue->tail = queue->head;
		}
		queue->size--;
		return node->thread;
	}
	return NULL;
}

my_pthread_t*
multiQueue_pop(Queue* queue[QUEUELEVEL]){
	if(!multiQueue_empty(queue)){

		scheduler.total_pendingThread--;

		my_pthread_t* thread;
		int i;
		for(i = QUEUELEVEL - 1; i >= 0; i--){
			thread = queue_pop(queue[i]);
			if(thread != NULL){
				return thread;
			}
		}
	}

	printf("[QUEUE] ERROR: Cannot pop, queue is empty!\n");
	return NULL;
}

/*
*	find a thread is in the queue?
*	@return 1 find, 0 not find
*/
int
findThread(Queue* queue, my_pthread_t* thread){
	if(queue_empty(queue)){
		return 0;
	}
	
	Node* node = queue->head->next;
	while(node != queue->tail){
		if(node->thread->_self_id == thread->_self_id){
			return 1;
		}
		node = node->next;
	}
	return 0;
}

void
queue_destory(Queue* queue){
	free(queue->head);
	free(queue->tail);
}

void
multiQueue_destory(Queue* queue[QUEUELEVEL]){
	int i;
	for(i = 0; i < QUEUELEVEL; i++ ){
		queue_destory(queue[i]);
	}
}

my_pthread_t*
queue_head(Queue* queue){
	if(!queue_empty(queue)){
		return queue->head->next->thread;
	}

	printf("[QUEUE] ERROR: queue is empty!");
	return NULL;
}

my_pthread_t*
multiQueue_head(Queue* queue[QUEUELEVEL]){
	if(!multiQueue_empty(queue)){
		return queue[QUEUELEVEL-1]->head->next->thread;
	}

	printf("[QUEUE] ERROR: queue is empty!");
	return NULL;
}

/*
*	Create Thread
*/
int 
my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, 
					void*(*function)(void*),void* arg){
	assert(scheduler.total_thread != MAX_THREADS);
	assert(getcontext(&thread->_ucontext_t) != -1);
	thread->func = function;
	thread->arg = arg;
	thread->_ucontext_t.uc_stack.ss_sp = thread->stack;
	thread->_ucontext_t.uc_stack.ss_size = sizeof(MIN_STACK);
	thread->_ucontext_t.uc_stack.ss_flags = 0;
	thread->_ucontext_t.uc_flags = 0;
	thread->_ucontext_t.uc_link = &scheduler.schedule_thread._ucontext_t;// &head->thread._ucontext_t;
	thread->state = READY;
	makecontext(&thread->_ucontext_t, function, 0);

	thread->_self_id = scheduler.total_thread++;
	thread->priority = QUEUELEVEL - 1;
	multiQueue_push(pendingThreadQueue,thread);
	printf("[PTHREAD] thread %d has been created!\n", thread->_self_id);
	return thread->_self_id;
}

/*
*	Yield thread 
*	Explicit call to end the pthread that the current context
*	be swapped out and another be scheduled
*/
void
my_pthread_yield(){
	printf("[PTHREAD] thread %d yield!\n", scheduler.runningThread->_self_id);
	if(scheduler.total_thread > 1){
		my_pthread_t* thread = scheduler.runningThread;
		thread->state = READY;
		my_pthread_t temp;
		memcpy(&temp, thread, sizeof(thread));
		getcontext(&thread->_ucontext_t);
		thread->func = temp.func;
		thread->arg = temp.arg;
		thread->_ucontext_t.uc_stack.ss_sp = temp._ucontext_t.uc_stack.ss_sp;
		thread->_ucontext_t.uc_stack.ss_size = temp._ucontext_t.uc_stack.ss_size;
		thread->_ucontext_t.uc_stack.ss_flags = temp._ucontext_t.uc_flags;
		thread->_ucontext_t.uc_flags = temp._ucontext_t.uc_flags;
		thread->_ucontext_t.uc_link = temp._ucontext_t.uc_link;
		setcontext(&scheduler.schedule_thread._ucontext_t);		
	}
}

/*
*	Exit thread
*/
void
my_pthread_exit(void* value_ptr){
	pid_t thread_id = *((pid_t *) value_ptr);
	//removeNode(thread_id);
}

/*
*	Call to the my_pthread_t library ensuring that the 
*	calling thread will not execute until the one it 
*	references exits. If value_ptr is not null, the return
*	value of the existing thread will be passed back
*	@para thread: 
*/
int
my_pthread_join(my_pthread_t thread, void**value_ptr){

	if(*value_ptr != NULL)
		return *((int*) value_ptr);
	return 0;
}

void 
schedule(){
	
	if(scheduler.total_pendingThread > 0){
		printf("[Scheduler] start to schedule! \n");
		my_pthread_t* currThread = multiQueue_pop(pendingThreadQueue);
		
		/*
			check how to process the popping thread
		*/
		switch(currThread->state){
			case READY:
				multiQueue_push(pendingThreadQueue, currThread);	
				break;
			case TERMINATED:
				queue_push(finishedThreadQueue, currThread);
				break;
			default:
				break;
		}

		scheduler.runningThread = currThread;
		currThread->state = RUNNING;
		printf("[Scheduler] switch to thread %d \n", currThread->_self_id);
		setcontext(&currThread->_ucontext_t);
	}else{
		printf("[Scheduler] finish all schedule!");
	}
}

/*
*	To handler the signal, switch to schedule function
*	@para sig: signal value
*	@para context: old_context
*/
void 
signal_handler(int sig, siginfo_t *siginfo, void* context){

	if(sig == SIGPROF){
		printf("[SignalHandler] receive scheduler signal!\n");
		//store the context of running thread
		
		 if(scheduler.runningThread->_self_id != -1){
			
			scheduler.runningThread->_ucontext_t.uc_mcontext = (*((ucontext_t*) context)).uc_mcontext;
			scheduler.runningThread->state = READY;
			/*
				Check CONTEXT_INTERRUPTION.
				if == true, do not decrease the priority
				otherwise, change the priority
			*/
			if(CONTEXT_INTERRUPTION){
				CONTEXT_INTERRUPTION = 0;
			}else{
				scheduler.runningThread->priority -= 1;
			}
		 }
		//go to the context of scheduler
		setcontext(&scheduler.schedule_thread._ucontext_t);
	}
}

/*
*	Initiate scheduler, signal and timer
*/

void
start(){
	/*
		init scheduler context;
	*/
	memset(&scheduler, 0, sizeof(schedule_t));
	scheduler.runningThread = (my_pthread_t*) malloc(sizeof(my_pthread_t));
	// scheduler.schedule_thread = (my_pthread_t*) malloc(sizeof(my_pthread_t));
	assert(getcontext(&scheduler.schedule_thread._ucontext_t) == 0);
	scheduler.schedule_thread._ucontext_t.uc_stack.ss_sp = scheduler.stack;
	scheduler.schedule_thread._ucontext_t.uc_stack.ss_size = sizeof(MIN_STACK);
	scheduler.schedule_thread._ucontext_t.uc_flags = 0;
	scheduler.schedule_thread._ucontext_t.uc_link = NULL;
	scheduler.runningThread->_self_id = -1;

	/*
		block timer interruption
	*/
	sigset_t sysmodel;sysmodel;
	sigemptyset(&sysmodel);
	sigaddset(&sysmodel, SIGPROF);
	scheduler.schedule_thread._ucontext_t.uc_sigmask = sysmodel;
	makecontext(&scheduler.schedule_thread._ucontext_t, schedule, 0);

	/*
		init multiple priority queue
	*/
	int i;
	for(i = 0; i < QUEUELEVEL; i++){
		pendingThreadQueue[i] = (Queue*) malloc(sizeof(Queue));
	}

	multiQueue_init(pendingThreadQueue);
	//scheduler.total_thread++;
	//scheduler.schedule_thread._self_id = scheduler.id++;
	//scheduler.schedule_thread.priority = ;
	//insertNode(createNode(scheduler.schedule_thread));
	//queue_push(pendingThreadQueue[QUEUELEVEL], scheduler.schedule_thread);
	/*
		Init signal
		once timer signal is trigger, it enters into sigal_handler
	*/
	struct sigaction act;
	memset (&act, 0, sizeof(act));
	act.sa_sigaction = signal_handler;
	act.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPROF,&act,NULL);
	
	/*
		Init timer
		it_value.tv_sec: first trigger delay
		it_interval.tv_sec: trigger interval
	*/
	struct itimerval tick;
	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = TIME_QUANTUM;
	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = TIME_QUANTUM;
	assert(setitimer(ITIMER_PROF,&tick,NULL)!=1);

	//testlock
	my_pthread_mutex_init(&countLock, NULL);
}

void
end(){
	multiQueue_destory(pendingThreadQueue);
	int i;
	for(i = 0; i < QUEUELEVEL; i++){
		free(pendingThreadQueue[i]);
	}
	free(scheduler.runningThread);
}

/*
*	Init lock
*/
int
my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){
	mutex->flag = 0;
	return 0;
}

/*
*	Lock  lock
*/
int
my_pthread_mutex_lock(my_pthread_mutex_t* mutex){
	printf("[PTHREAD] thread %d enter the lock\n",scheduler.runningThread->_self_id);
	while(__sync_lock_test_and_set(&mutex->flag,1)){
		printf("[PTHREAD] thread %d spins in the lock\n",scheduler.runningThread->_self_id);
		my_pthread_yield();
	}
	return 0;
}

/*
*	Unlock lock
*/
int 
my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){
	__sync_lock_release(&mutex->flag);
	printf("[PTHREAD] thread %d unlock!\n",scheduler.runningThread->_self_id);
	return 0;
}

/*
*	Destory lock
*/
int 
my_pthread_mutex_destory(my_pthread_mutex_t* mutex){
	return 0;
}

/*
*	Test function 1: loops permanently	
*/
void*
test(){
	printf("[TEST] thread %d is running\n", scheduler.runningThread->_self_id);
	int a = scheduler.runningThread->_self_id*1000000;
	while(1){
		if(a%1000000 == 0)
			printf("%d\n",a/1000000);
		a++;
	}
	return NULL;
}


/*
*	Test function 2: test lock
*/
static int sb = 0;
void*
test2(){
	printf("[TEST] thread %d is running\n", scheduler.runningThread->_self_id);
	static int enterTime = 0;
	enterTime++;
	printf("enter time %d\n", enterTime);

	my_pthread_mutex_lock(&countLock); 
	while(enterTime < 3){
		sb++;
		//printf("%d\n",sb);
	}
	my_pthread_mutex_unlock(&countLock); 

	return NULL;
}

void*
test3(){
	while(1){
		printf("33333\n");
	}
	return NULL;
}

int
main(){
	start();

	my_pthread_t thread;
	my_pthread_t thread2;
	my_pthread_t thread3;

	my_pthread_create(&thread,NULL,&test2,NULL);
	my_pthread_create(&thread2,NULL,&test2,NULL);
	my_pthread_create(&thread3,NULL,&test2,NULL);

	while(1){
		//
	}
	end();
	printf("exit program\n");
	return 0;
}