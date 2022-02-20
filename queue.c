#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    element_t *q = malloc(sizeof(element_t));
    if (!q)
        return NULL;
    INIT_LIST_HEAD(&q->list);
    return &q->list;
}
/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    struct list_head *node = l->next;
    while (node != l) {
        // cppcheck-suppress nullPointer
        element_t *e = container_of(node, element_t, list);
        node = node->next;
        q_release_element(e);
    }
    // cppcheck-suppress nullPointer
    element_t *head = container_of(l, element_t, list);
    free(head);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *q = malloc(sizeof(element_t));
    if (!q)
        return false;
    int charsize = 0;
    while (*(s + charsize))
        charsize += 1;
    q->value = malloc(charsize + 1);
    if (q->value == NULL) {
        free(q);
        return false;
    }
    strncpy(q->value, s, charsize);
    q->value[charsize] = '\0';
    list_add(&q->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *q = malloc(sizeof(element_t));
    if (!q)
        return false;
    int charsize = 0;
    while (*(s + charsize))
        charsize += 1;
    q->value = malloc(charsize + 1);
    if (q->value == NULL) {
        free(q);
        return false;
    }
    strncpy(q->value, s, charsize);
    q->value[charsize] = '\0';
    list_add_tail(&q->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    if (!sp)
        return NULL;
    // cppcheck-suppress nullPointer
    element_t *cur_e = list_entry(head->next, element_t, list);
    list_del_init(head->next);
    strncpy(sp, cur_e->value, bufsize - 1);
    sp[bufsize - 1] = '\0';
    return cur_e;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    if (!sp)
        return NULL;
    // cppcheck-suppress nullPointer
    element_t *cur_e = list_entry(head->prev, element_t, list);
    list_del_init(head->prev);
    strncpy(sp, cur_e->value, bufsize - 1);
    sp[bufsize - 1] = '\0';
    return cur_e;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;

    if (list_is_singular(head)) {
        head = head->next;
        // cppcheck-suppress nullPointer
        element_t *e = container_of(head, element_t, list);
        list_del(head);
        q_release_element(e);
        return true;
    }

    int del_n_index = 0;
    struct list_head *node = head->next;
    /* even nodes  */
    if (q_size(node) % 2 == 0) {
        del_n_index = q_size(node) / 2;
    } else { /* odd nodes  */
        del_n_index = (q_size(node) + 1) / 2 - 1;
    }
    while (del_n_index > 0) {
        node = node->next;
        del_n_index--;
    }
    // cppcheck-suppress nullPointer
    element_t *e = container_of(node, element_t, list);
    list_del(node);
    q_release_element(e);
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *node, *del;
    element_t *del_node;
    node = head->next;
    while (node != head) {
        char *cstr, *nstr;
        // cppcheck-suppress nullPointer
        cstr = list_entry(node, element_t, list)->value;
        if (node->next == head) {
            break;
        }
        // cppcheck-suppress nullPointer
        nstr = list_entry(node->next, element_t, list)->value;
        if (strcmp(cstr, nstr) == 0) {
            del = node;
            node = node->next;
            list_del_init(del);
            // cppcheck-suppress nullPointer
            del_node = list_entry(del, element_t, list);
            q_release_element(del_node);
        } else {
            node = node->next;
        }
    }
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    struct list_head *odd, *even;
    if (!head)
        return;
    odd = head->next;
    even = odd->next;
    while (true) {
        // swap odd and even
        odd->next = even->next;
        odd->next->prev = odd;
        even->prev = odd->prev;
        even->prev->next = even;
        odd->prev = even;
        even->next = odd;
        // end
        odd = odd->next;
        even = odd->next;
        if (odd == head || even == head) {
            break;
        }
    }
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *node, *temp;
    for (node = head->next; node != head; node = node->prev) {
        temp = node->next;
        node->next = node->prev;
        node->prev = temp;
    }
    temp = head->next;
    head->next = head->prev;
    head->prev = temp;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */

struct list_head *mergeTwoLists(struct list_head *left, struct list_head *right)
{
    struct list_head *head = NULL, **ptr = &head, **node;

    for (node = NULL; left && right; *node = (*node)->next) {
        // cppcheck-suppress nullPointer
        node = (strcmp(list_entry(left, element_t, list)->value,
                       // cppcheck-suppress nullPointer
                       list_entry(right, element_t, list)->value) < 0)
                   ? &left
                   : &right;
        *ptr = *node;
        ptr = &(*ptr)->next;
    }
    *ptr = left ? left : right;

    return head;
}

struct list_head *mergesort(struct list_head *head)
{
    if (!head->next)
        return head;

    struct list_head *fast = head, *slow = head, *mid;
    while (true) {
        if (fast->next == NULL || fast->next->next == NULL)
            break;
        fast = fast->next->next;
        slow = slow->next;
    }

    mid = slow->next;
    slow->next = NULL;

    struct list_head *left = mergesort(head), *right = mergesort(mid);
    return mergeTwoLists(left, right);
}

void q_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *node = head->next, *temp;

    head->prev->next = NULL;
    head->next = NULL;

    node = mergesort(node);

    temp = head;
    temp->next = node;
    while (temp->next) {
        temp->next->prev = temp;
        temp = temp->next;
    }
    temp->next = head;
    head->prev = temp;
}
