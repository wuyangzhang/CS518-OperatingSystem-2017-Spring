#ifndef __Priority_Queue_H
#define __Priority_Queue_H

#include "my_pthread_t.h"
#include <stdio.h>

#define numqueuelevel 4

typedef struct link{
	struct link *prev, *next;
} link_t;

typedef struct Node{
	my_pthread_t thread;
	struct Node *prev, *next;
} Node_t;

static Node_t priorityQ[numqueuelevel];

static inline void Node_init(Node_t *head) {
	head->prev = head->next = head;
}

static inline int Node_is_equal(Node_t *one, Node_t *two){
	if(one == two){
		return 1;
	}
	return -1;
}

static inline void __Node_add(Node_t *prev, Node_t *next, Node_t *curr) {
	prev->next = next->prev = curr;
	curr->prev = prev;
	curr->next = next;
}

static inline void Node_add_by_priority(Node_t *tail, Node_t *curr) {
	__Node_add(tail, tail->next, curr);
}

static inline Node_t* Node_remove(Node_t *get){
	get->prev->next = get->next;
	get->next->prev = get->prev;
	get->prev = get->next = get;
	return get;
}

#define queue_for_each(head, elem) \
	for ((elem) = (head)->next; (elem) != (head); (elem) = (elem)->next)

#define queue_entry(elem, type, pos) \
	(type *)(((char *)elem)-((char *)&((type *)0)->pos))

static inline void queue_init(Node_t *priori){
	int i;
	for(i = 0; i < numqueuelevel; ++i){
		Node_init(priori + i);
	}
} 

static inline void queue_push(my_pthread_t *curr, Node_t *priori){
	Node_t *new = (Node_t*) malloc(sizeof(Node_t));
	new->thread = *curr;
	Node_add_by_priority((priori + new->thread.priority)->next, new);
}

static inline void queue_look_for_each(Node_t *priori){
	printf("\n************priority queue****************");
	int i;
	Node_t *elem_iter;
	for (i = 0; i < numqueuelevel; ++i)
	{
		/* check the insertation */
		printf("\n---------priority level %d------------\n", i);
		Node_t *iterator;
		queue_for_each(priori + i, iterator){
//			elem_iter = queue_entry(iterator, contexts_t, link);
			printf(">>>>>>thread id %d", iterator->thread._self_id);
		}
	}
}

static inline my_pthread_t* queue_pop(int priority, Node_t *priori) {
	if(Node_is_equal(priori + priority, (priori + priority)->next) == 1){
		printf("I do not have any element in my link table, why you call me!!!\n");
		return NULL;
	}
//	link_t *elem = link_remove(priorityQ[priority].next);	
	Node_t *context = (priori + priority)->next;
	printf("got thread id %d from priority level %d", context->thread._self_id, context->thread.priority);
	return &(context->thread);
}

static inline int queue_remove(int priority, Node_t *priori){
	if(Node_is_equal(priori + priority, (priori + priority)->next) == 1){
                printf("I do not have any element in my link table, why you remove me!!!\n");
                return -1;
        }
	//TODO: return Node
	Node_t *elem = Node_remove((priori + priority)->next);
	return 1;
}

//static inline void queue_pop_by_id(int thread_id){
//	contexts_t *elem_iter;
//	int i;
//	for (i = 0; i < numqueuelevel; ++i)
//	{
//		/* look up thread with thread id */
//		link_t *iterator;
//		queue_for_each(&priorityQ[i], iterator){
//			elem_iter = queue_entry(iterator, contexts_t, link);
//			if(elem_iter->thread._self_id == thread_id){
//				printf("found it at %d!\n", i);
//				link_remove(&elem_iter->link);
//			}
//			printf("This is prior level %d", i);
//		}
//	}
//}

void testing()
{
	int i, j;
	
	//initialize
	queue_init(priorityQ);
	my_pthread_t c1, c2, c3;

	/* high priority -> low priority: 0 -> 3*/
	c1.priority = 1;
	c1._self_id = 1;
	c2.priority = 1;
	c2._self_id = 2;
	c3.priority = 2;
	c3._self_id = 3;

//	link_add_by_priority(priorityQ[c1.thread.priority].next, &(c1.link));
//	link_add_by_priority(priorityQ[c2.thread.priority].next, &(c2.link));
//	link_add_by_priority(priorityQ[c3.thread.priority].next, &(c3.link));

	queue_push(&c1, priorityQ);
	queue_push(&c2, priorityQ);
	queue_push(&c3, priorityQ);
	
//	printf("thread %d, priority %d\n",c1.thread._self_id, c1.thread.priority);
//	printf("thread %d, priority %d\n",c2.thread._self_id, c2.thread.priority);
//	printf("thread %d, priority %d\n",c3.thread._self_id, c3.thread.priority);

	queue_look_for_each(priorityQ);

//	queue_pop_by_id(1);
	queue_pop(0, priorityQ);
	
	queue_remove(2, priorityQ);
	queue_look_for_each(priorityQ);
}

int main(int argc, char const *argv[])
{
	/* code */
	testing();
	return 0;
}
#endif
