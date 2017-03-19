/* Copyright (C) Wuyang Zhang Feb 17 2017
	Operating System Project 2
 */

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "my_pthread_t.h"

int debug = 0;
static schedule_t scheduler;
static int counter = 0;
static my_pthread_mutex_t countLock;
static int CONTEXT_INTERRUPTION = 0;
static Queue* pendingThreadQueue[QUEUELEVEL];
static Queue* finishedThreadQueue;
static Queue* scanThreadQueue;
static my_pthread_t mainThread;

clock_t timeStart, timeEnd;
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
    if(debug){
        printf("Push thread %d state(%d) priority(%d), pending %d\n", thread->_self_id,thread->state,thread->priority ,scheduler.total_pendingThread);
    }
}

static inline void
queue_push(Queue* queue, my_pthread_t* thread){
    Node* node = (Node*)malloc(sizeof(Node));
    node->thread = thread;
    node->next = NULL;
    
    queue->tail->next = node;
    queue->tail = node;
    
    queue->size++;
}

static inline my_pthread_t*
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
                if(debug){
                    printf("Pop thread %d state(%d) priority(%d), pending %d\n", thread->_self_id,thread->state,thread->priority ,scheduler.total_pendingThread);
                }
                return thread;
            }
        }
    }
    if(debug){
        printf("[QUEUE] ERROR: Cannot pop, queue is empty!\n");
    }
    return NULL;
}

/*
 *	find a thread is in the queue?
 *	@return 1 find, 0 not find
 */
int
queue_findThread(Queue* queue, my_pthread_t* thread){
    if(queue_empty(queue)){
        return 0;
    }
    
    Node* node = queue->head->next;
    while(node != queue->tail->next){
        if(node->thread->_self_id == thread->_self_id){
            return 1;
        }
        node = node->next;
    }
    return 0;
}

static inline void
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

void
queue_print(Queue* queue){
    if(queue_empty(queue)){
        return;
    }
    Node* node = queue->head->next;
    while(node != queue->tail->next){
        if(debug){
            printf("thread id %d: state(%d), priority(%d)\n", node->thread->_self_id, node->thread->state, node->thread->priority);
        }
        node = node->next;
    }
}

void
multiQueue_print(Queue* queue[QUEUELEVEL]){
    //printf("\t****************print all %d thread in queue!****************\n", scheduler.total_pendingThread);
    int i;
    for(i = QUEUELEVEL - 1; i >= 0; i--){
        queue_print(queue[i]);
    }
    printf("\n");
}

my_pthread_t*
queue_head(Queue* queue){
    if(!queue_empty(queue)){
        return queue->head->next->thread;
    }
    
    //printf("[QUEUE] ERROR: queue is empty!");
    return NULL;
}

my_pthread_t*
multiQueue_head(Queue* queue[QUEUELEVEL]){
    if(!multiQueue_empty(queue)){
        return queue[QUEUELEVEL-1]->head->next->thread;
    }
    
    //printf("[QUEUE] ERROR: queue is empty!");
    return NULL;
}


/*
 *	Exit thread
 */
void
my_pthread_exit(void* value_ptr){
    if(debug){
        printf("[PTHREAD] Thread %d exit!\n", scheduler.runningThread->_self_id);
    }
    
    scheduler.runningThread->state = TERMINATED;
    setcontext(&scheduler.schedule_thread._ucontext_t);
    

    
}

/*
 *	framework to start up thread
 */
void
my_pthread_startup_function(void*(*function(void*)), void* arg){
    if(debug){
        printf("[PTHREAD] thread %d launch  start up function\n", scheduler.runningThread->_self_id);
    }
    function(arg);
    my_pthread_exit(NULL);
}

/*
 *	Create Thread
 */
int
my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr,
                  void*(*function)(void*),void* arg){
    assert(scheduler.total_thread != MAX_THREADS);
    assert(getcontext(&thread->_ucontext_t) != -1);
    
    thread->_ucontext_t.uc_stack.ss_sp = thread->stack;
    thread->_ucontext_t.uc_stack.ss_size = MIN_STACK;
    thread->_ucontext_t.uc_stack.ss_flags = 0;
    thread->_ucontext_t.uc_link = &scheduler.schedule_thread._ucontext_t;
    thread->func = function;
    thread->arg = arg;
    thread->state = READY;
    thread->_self_id = scheduler.total_thread++;
    thread->priority = QUEUELEVEL - 1;
    thread->runningCounter = 0;
    makecontext(&thread->_ucontext_t,my_pthread_startup_function, 2,  function, arg);
    multiQueue_push(pendingThreadQueue,thread);
    
    if(debug){
        printf("[PTHREAD] thread %d has been created!\n", thread->_self_id);
    }
    
    /* set all frame unused */
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        thread->pageTable[i] = (pageDir*)malloc(sizeof(pageDir));
        thread->pageTable[i]->in_memory_state = 0;
        thread->pageTable[i]->in_use_state = 0;
        thread->pageTable[i]->page_redirect = -1;
    }
    int frame = find_free_frame();
    if(frame!=-1){
        printf("[PTHREAD] Allocate Page %d to Thread %d!\n", frame,thread->_self_id);
        allocateFrame(thread, frame);
    }else{
        perror("Memory Full!");
    }
    /* init available frame for this thread */
//    thread->currentPage = 0;
//    allocate_frame(thread);
    
    return thread->_self_id;
}

/*
 *	Yield thread
 *	Explicit call to end the pthread that the current context
 *	be swapped out and another be scheduled
 */
void
my_pthread_yield(){
    //printf("[PTHREAD] thread %d yield!\n", scheduler.runningThread->_self_id);
    
    swapcontext(&scheduler.runningThread->_ucontext_t, &scheduler.schedule_thread._ucontext_t);
    
    /*
     my_pthread_t* thread = scheduler.runningThread;
     thread->state = WAITING;
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
     */
    
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
    
    while(!queue_findThread(finishedThreadQueue, &thread)){
        //printf("[PTHREAD] thread %d waits for thread %d to finish\n", scheduler.runningThread->_self_id, thread._self_id);
        my_pthread_yield();
    }
    return 0;
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
    //printf("[PTHREAD] thread %d enter the lock\n",scheduler.runningThread->_self_id);
    while(__sync_lock_test_and_set(&mutex->flag,1)){
        //printf("[PTHREAD] thread %d spins in the lock\n",scheduler.runningThread->_self_id);
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
    //printf("[PTHREAD] thread %d unlock!\n",scheduler.runningThread->_self_id);
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
	1.Loop the whole queue push all of them into scan queue
	2.If a threshold's running time is smaller than a threshold, its priority goes higher
	3.Reset running time;
 */
void
threadPriority_scan(Queue* queue[QUEUELEVEL], int threshold){
    
    //printf("\t*************Adjust priority of all %d thread in queue!***********\n", scheduler.total_pendingThread);
    while(!multiQueue_empty(queue)){
        my_pthread_t* thread = multiQueue_pop(queue);
        if(thread->runningCounter < threshold){
            if(thread->priority < QUEUELEVEL -1){
                thread->priority++;
            }
        }
        
        thread->runningCounter = 0;
        queue_push(scanThreadQueue, thread);
    }
    
    while(!queue_empty(scanThreadQueue)){
        my_pthread_t* thread = queue_pop(scanThreadQueue);
        multiQueue_push(queue, thread);
    }
}

void
schedule(){
    static int round = 1;
    if(debug){
        printf("\n****************************schedule round %d**************************\n\n", round++);
    }
    multiQueue_print(pendingThreadQueue);
    
    /*
     Handle current running thread
     */
    if(debug){
        printf("runningThread %d state: %d\n", scheduler.runningThread->_self_id, scheduler.runningThread->state);
    }
    switch(scheduler.runningThread->state){
        case(READY)://system interruption
            multiQueue_push(pendingThreadQueue, scheduler.runningThread);
            break;
        case(RUNNING)://corner case, yield interruption
            scheduler.runningThread->state = READY;
            multiQueue_push(pendingThreadQueue, scheduler.runningThread);
            break;
        case(RESUME):
            scheduler.runningThread->state = RUNNING;
            setcontext(&scheduler.runningThread->_ucontext_t);
            break;
        case(WAITING)://thread yield
            multiQueue_push(pendingThreadQueue, scheduler.runningThread);
            break;
        case(TERMINATED)://thread terminate
            queue_push(finishedThreadQueue, scheduler.runningThread);
            break;
        default:
            break;
    }
    /*
     Handle the next running thread
     */
    if(scheduler.total_pendingThread > 0){
        /*
         Periodically adjust priority of threads
         */
        if(round % (scheduler.total_pendingThread * QUEUECHECK + 1) == 0){
            threadPriority_scan(pendingThreadQueue,1);
        }
        // Unprotect the pages before context switch
        if(scheduler.runningThread->_self_id!=-1){
            page_protect(1);
        }
        
        my_pthread_t* currThread = multiQueue_pop(pendingThreadQueue);
        scheduler.runningThread = currThread;
        if(debug){
            printf("[Scheduler] start to schedule! \n");
            printf("\n************************************************\n\n");
            printf("[Scheduler] switch to thread %d \n", currThread->_self_id);
        }
        
        currThread->state = RUNNING;
        currThread->runningCounter++;
        
        // Protect the pages after context switch
        page_protect(0);
        printf("[Scheduler] switch to thread %d \n", currThread->_self_id);

        setcontext(&currThread->_ucontext_t);
    }else{
        timeEnd = clock() - timeStart;
        int msec = timeEnd * 1000000 / CLOCKS_PER_SEC;
        
        printf("[Scheduler] finish all schedule in %d seconds %d ms", msec / 1000, msec % 1000);
        
        //setcontext(&mainThread._ucontext_t);
        
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
        if(debug){
            printf("[SignalHandler] receive scheduler signal!\n");
        }
        //store the context of running thread
        
        if(scheduler.runningThread->_self_id != -1){
            
            scheduler.runningThread->_ucontext_t.uc_mcontext = (*((ucontext_t*) context)).uc_mcontext;
            
            if(scheduler.runningThread->state == RUNNING){
                scheduler.runningThread->state = READY;
            }
            /*
             Check CONTEXT_INTERRUPTION.
             if == true, do not decrease the priority
             otherwise, change the priority
             */
            if(CONTEXT_INTERRUPTION){
                CONTEXT_INTERRUPTION = 0;
                scheduler.runningThread->state = RESUME;
            }else{
                if(scheduler.runningThread->priority >= 1){
                    if(debug){
                        printf("thread %d 's priority has been degraded from %d to %d", scheduler.runningThread->_self_id, scheduler.runningThread->priority, scheduler.runningThread->priority-1);
                    }
                    scheduler.runningThread->priority -= 1;
                }
            }
        }
        
        //go to the context of scheduler
        setcontext(&scheduler.schedule_thread._ucontext_t);
    }
    
}

/*
 * alloc a frame
 */
void allocateFrame(my_pthread_t* thread, int frame){
    printf("Allocate Page %d to thread %d.\n",frame,thread->_self_id);
    thread->pageTable[frame]->in_memory_state = 1;
    thread->pageTable[frame]->in_use_state = 1;
    thread->pageTable[frame]->page_redirect = frame;
    setPageTableThreadPtr(thread->pageTable[frame], frame);
    thread->currentUsePage = frame;
    //SET_USE_PAGE(pages[frame]);
}
//
///*
// * update frame
// */
//void updateFrame(my_pthread_t* thread, int frame){
//    thread->currentUsePage = frame;
//    thread->pageTable[frame] = frame;
//}
/*
 *  -getUsingFrame
 *  @Return page using table to memory
 */
int* getUsingFrame(my_pthread_t* pthread){
    return pthread->pageTable;
}

/*
 * -getCurrentRunningThread
 */
my_pthread_t* getCurrentRunningThread(){
    return scheduler.runningThread;
}


/*
 *	Initiate scheduler, signal and timer
 */
static inline void
scheduler_init(){
    memset(&scheduler, 0, sizeof(schedule_t));
    scheduler.runningThread = (my_pthread_t*) malloc(sizeof(my_pthread_t));
    memset(scheduler.runningThread, 0, sizeof(my_pthread_t));
    // scheduler.schedule_thread = (my_pthread_t*) malloc(sizeof(my_pthread_t));
    assert(getcontext(&scheduler.schedule_thread._ucontext_t) == 0);
    //scheduler.schedule_thread.stack = malloc(sizeof(MIN_STACK));
    scheduler.schedule_thread._ucontext_t.uc_stack.ss_sp = scheduler.schedulerStack;
    scheduler.schedule_thread._ucontext_t.uc_stack.ss_size = MIN_STACK;
    //scheduler.schedule_thread._ucontext_t.uc_flags = 0;
    scheduler.schedule_thread._ucontext_t.uc_link = NULL;
    scheduler.schedule_thread._self_id = -1;
    scheduler.schedule_thread.state = -1;
    scheduler.schedule_thread.priority = QUEUELEVEL-1;
    /*
     block timer interruption
     */
    sigset_t sysmodel;
    sigemptyset(&sysmodel);
    sigaddset(&sysmodel, SIGPROF);
    scheduler.schedule_thread._ucontext_t.uc_sigmask = sysmodel;
    makecontext(&scheduler.schedule_thread._ucontext_t, schedule, 0);
    scheduler.runningThread = &scheduler.schedule_thread;
    
}

static inline void
threadQueue_init(){
    int i;
    for(i = 0; i < QUEUELEVEL; i++){
        pendingThreadQueue[i] = (Queue*) malloc(sizeof(Queue));
        memset(pendingThreadQueue[i], 0, sizeof(Queue));
    }
    
    multiQueue_init(pendingThreadQueue);
    
    /*
     init finished thread queue
     */
    finishedThreadQueue = (Queue*) malloc(sizeof(Queue));
    memset(finishedThreadQueue, 0, sizeof(Queue));
    queue_init(finishedThreadQueue);
    
    scanThreadQueue = (Queue*) malloc(sizeof(Queue));
    memset(scanThreadQueue, 0, sizeof(Queue));
    queue_init(scanThreadQueue);
}

static inline void
signal_init(){
    
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
    
}

void
start(){
    /*
     init scheduler context;
     */
    scheduler_init();
    
    /*
     init multiple priority queue
     */
    threadQueue_init();
    
    /*
     Init signal
     once timer signal is trigger, it enters into sigal_handler
     */
    signal_init();
    
    file_init();
    
    seg_fault_handle_init();
    
    /*
     init memory
     */
    if(mem_init() != 0){
        perror("memory init fail!\n");
    }

    //testlock
    my_pthread_mutex_init(&countLock, NULL);
}

void
end(){
    multiQueue_destory(pendingThreadQueue);
    
    free(scheduler.runningThread);
}

int sum(int num){
    if(num != 0){
        //printf("curr num %d\n", num);
        return num + sum(num-1);
    }else{
        //printf("curr num %d\n", num);
        return num;
    }
}

void*
test1(int thread_num){
    int static times = 0;
    times ++;
    int* p;
    p = (int*)myallocate(PAGE_SIZE-100, __FILE__, __LINE__, 1);
    *p = 520;
    //strcpy(p, "HelloHelloHelloHelloHelloHello");
    
    while(times<thread_num){
        
    }
    times ++;
    if(getCurrentRunningThread()->_self_id == 0){
        char* p1 = (char*)myallocate(PAGE_SIZE-100, __FILE__, __LINE__, 1);
        strcpy(p1, "WorldWorldWorldWorldWorldWorld");
    }
    if(getCurrentRunningThread()->_self_id == 1){
        //char* p1 = (char*)myallocate(PAGE_SIZE-100, __FILE__, __LINE__, 1);
        int x = *p*10;
        printf("X equals to %d\n",x);
    }
    
    while(times < 2*thread_num){
        
    }
    
    //mydeallocate(p,  __FILE__, __LINE__, 1);

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

int
main(){
    
    
    int threadNum = 5;
    
    start();
    timeStart = clock();
    
    my_pthread_t thread[threadNum];
    
    int i;
    for(i = 0; i < threadNum; i++){
        my_pthread_create(&thread[i], NULL, &test1, threadNum);
    }
    
    while(1){
       
    }
    
    
    
    timeStart = clock();
    /*
     pthread_t t[threadNum];
     int i;
     for(i = 0; i < threadNum; i++){
     pthread_create(&t[i], NULL, test1, 1000000);
     }
     
     timeEnd = clock() - timeStart;
     int msec = timeEnd * 1000000 / CLOCKS_PER_SEC;
     printf("[Scheduler] finish all schedule in %d ms %d us\n", msec / 1000, msec % 1000);
     
     
     for(i = 0; i < threadNum; i++){
     pthread_join(&t[i], NULL);
     }
     
     //end();
     printf("exit program\n");
     */
    return 0;
}
