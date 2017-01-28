// Author: John-Austen Francisco
// Date: 9 September 2015
//
// Preconditions: Appropriate C libraries
// Postconditions: Generates Segmentation Fault for
//                               signal handler self-hack

// Student name: Wuyang Zhang
// Ilab machine used: cd.cs.rutgers.edu


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void segment_fault_handler(int signum)
{
	//signum indicates why type of signal occurred
	//printf("signum is %p\n", (void *) &signum);
		printf("I am slain!\n");
		/*
		  addr signum: 0xffffcbf0
		  fault code addr: 0x0804849b,
		*/
	     		//Use the signnum to construct a pointer to flag on stored stack
	//Increment pointer down to the stored PC
	//Increment value at pointer by length of bad instruction
	       	*(int *)(&signum +  (68 - 8) / 4) += 2;
	
	


}


int main()
{
	int r2 = 0;

	signal(SIGSEGV, segment_fault_handler);

	r2 = *( (int *) 0 );
	
	printf("I live again!\n");

	return 0;
}
