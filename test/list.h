#ifndef __LIST_H
#define __LIST_H


typedef struct list_head {
	struct list_head *prev, *next;
} list_t;

static inline void list_init(list_t *head) {
	head->prev = head->next = head;
}

static inline void __list_add(list_t *prev, list_t *next, list_t *curr) {
	prev->next = next->prev = curr;
	curr->prev = prev;
	curr->next = next;
}

static inline void list_add_after(list_t *head, list_t *add) {
	__list_add(head, head->next, add);
}

static inline void list_add_before(list_t *head, list_t *add) {
	__list_add(head->prev, head, add);
}


static inline void list_remove(list_t *rem) {
	rem->prev->next = rem->next;
	rem->next->prev = rem->prev;
	rem->prev = rem->next = rem;
}

#define list_for_each(head, elem) \
	for ((elem) = (head)->next; (elem) != (head); (elem) = (elem)->next)

#define list_entry(elem, type, pos) \
	(type *)(((char *)elem)-((char *)&((type *)0)->pos))


#endif
