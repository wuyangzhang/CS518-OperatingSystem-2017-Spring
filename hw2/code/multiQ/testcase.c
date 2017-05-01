#include <pthread.h>

#include "my_pthread_t.h"

/*
*	Test function 1: recursive function
*/

int sum(int num){
	if(num != 0){
		return num + sum(num-1);
	}else{
		return num;
	}
}

void*
test1_my_pthread(){
	printf("[TEST] thread %d is running\n", scheduler.runningThread->_self_id);
	int a = scheduler.runningThread->_self_id*1000000;
	while(1){
		if(a%1000000 == 0)
			//printf("%d\n",a/1000000);
		a++;
	}
	return NULL;
}

void*
test1_pthread(){

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
		//printf("33333\n");
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

	getcontext(&mainThread._ucontext_t);
	
	int i = 0;
	while( i < 10000){
		i++;
		printf("i value %d\n", i);
	}
	
	// my_pthread_join(thread, NULL);
	// my_pthread_join(thread2, NULL);
	// my_pthread_join(thread3, NULL);
	
	//end();
	printf("exit program\n");
	return 0;
}
