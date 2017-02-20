#include "datalist.h"
#include <stdio.h>

int main(int argc, char **argv) {
	list_t head;
	list_t *iterator;
	element_t *elem_iterator;
//	list_t e1, e2, e3;
//	printf("pos e1: %p\n", &e1);
//	printf("pos e2: %p\n", &e2);
//	printf("pos e3: %p\n", &e3);
//
//
//	list_init(&head);
//
//	list_add_first(&head, &e1);
//	list_add_first(&head, &e2);
//	list_add_last(&head, &e3);
//
//	list_for_each(&head, iterator) {
//		printf("pos: %p\n", iterator);
//	}

	list_t head2;

	element_t e1, e2, e3;
	printf("pos e1: %p\n", &e1);
	printf("pos e2: %p\n", &e2);
	printf("pos e3: %p\n", &e3);

	list_init(&head);
	list_add_after(&head, &e1.list);	
	list_add_after(&head, &e2.list);	
	list_add_before(&head, &e3.list);	
	list_for_each(&head, iterator) {
		printf("pos: %p\n", iterator);
		elem_iterator = list_entry(iterator, element_t, list);
		printf("pos_elem: %p\n", elem_iterator);
	}

	list_init(&head2);
	list_add_after(&head2, &e1.list2);
	list_add_after(&head2, &e3.list2);	
	list_add_after(&head2, &e2.list2);	
	list_for_each(&head2, iterator) {
		printf("pos: %p\n", iterator);
		elem_iterator = list_entry(iterator, element_t, list2);
		printf("pos_elem: %p\n", elem_iterator);
	}

	element_t to_add;
	list_for_each(&head2, iterator) {
		elem_iterator = list_entry(iterator, element_t, list2);
		if (elem_iterator->id > to_add.id) {
			//insert before;
			list_add_before(iterator, &to_add.list2);
		}
	}
	if (iterator != head2) {
		list_add_before(head2, &to_add.list2);
		// insert last
	}

}
