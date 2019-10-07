/* 
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Modified to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
    queue_t *q =  malloc(sizeof(queue_t));
    /* What if malloc returned NULL? */
    if(q == NULL) return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->siz = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    /* How about freeing the list elements and the strings? */
    /* Free queue structure */
    if(q == NULL) return ;
    list_ele_t *p = q->head;
    while(p != NULL){
      list_ele_t *tmp = p;
      p = p->next;
      free(tmp->value);
      free(tmp);
    }
    free(q);
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh;
    /* What should you do if the q is NULL? */
  //  q = malloc(sizeof(queue_t));
    if(q == NULL) return false;
    newh = malloc(sizeof(list_ele_t));
    /* Don't forget to allocate space for the string and copy it */
    /* What if either call to malloc returns NULL? */
    if(newh == NULL) return false;
    newh->value = malloc(sizeof(s));
    if((newh->value) == NULL){
      free(newh);
      return false;
    } 
    strcpy(newh->value, s);
    newh->next = q->head;
    if((q->head) == NULL) q->tail = newh;
    q->head = newh;
    (q->siz)++;
    return true;
}


/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    /* You need to write the complete code for this function */
    /* Remember: It should operate in O(1) time */
    list_ele_t *newh;
    if(q == NULL) return false;

    newh = malloc(sizeof(list_ele_t));
    if(newh == NULL) return false;
    newh->value = malloc(sizeof(s));
    if((newh->value) == NULL){
      free(newh);
      return false;
    } 
    strcpy(newh->value, s);
    newh->next = NULL;
    if((q->head) == NULL){
      q->tail = newh;
      q->head = newh;
    }
    q->tail->next = newh;
    q->tail = newh;
    q->siz++;
    return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to *sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    /* You need to fix up this code. */
    if(q == NULL || (q->head) == NULL) return false;
    list_ele_t *tmp = q->head;
    if(sp != NULL) strcpy(sp, tmp->value);
    free(tmp->value);
    q->head = q->head->next;
    free(tmp);
    (q->siz)--;
    return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    /* You need to write the code for this function */
    /* Remember: It should operate in O(1) time */
    if(q == NULL) return 0;
    return q->siz;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */

list_ele_t* e_reverse(list_ele_t *p){
  if((p->next) == NULL) return p;
  list_ele_t *res = e_reverse(p->next);
  list_ele_t *it;
  for(it = res; (it->next) != NULL; it = it->next);
  it->next = p; p->next = NULL;
  return res;
}

void q_reverse(queue_t *q)
{
    /* You need to write the code for this function */
    if(q == NULL || (q -> head) == NULL) return ;
    q->tail = q->head;
    q->head = e_reverse(q->head);
    return ;
}

/*
void q_print(queue_t *q){
  list_ele_t *p = q->head;
  while(p != NULL){
    printf("%s, ", p->value);
    p = p->next;
  }
  printf("\n");
}

int main(){
  queue_t *que = q_new();
  q_insert_tail(que, "ffff");
  q_insert_tail(que, "pdf");
  q_insert_head(que, "what");

  q_print(que);

  q_reverse(que);
}*/
