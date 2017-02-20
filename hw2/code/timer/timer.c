#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>
void
scheduleAlgorithm(){
	//look for the thread with the highest priority
	printf("%s\n","hello");
}

int main(){
	//scheduling();
	
	signal(SIGALRM,scheduleAlgorithm);

	struct itimerval tick;
	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 1;
	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = 10000;
	assert(setitimer(ITIMER_REAL,&tick,NULL)!=1);

	while(1){
		pause();
	}
	//my_pthread_create(thread, NULL, test, NULL);
	return 0;
}