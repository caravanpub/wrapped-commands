/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/
#ifndef LIST_H
#define LIST_H

struct NODE {
  struct NODE *prev;
  struct NODE *next;
  void* data;
};

typedef struct NODE node;

struct LIST {
  node* head;
  node* tail;
  node* current;
  int count;
};

typedef struct LIST list;

/* create lists, add nodes, delete nodes, and other basic list operations.
   NONE of these change the current pointer */

/* node foo */
node* new_node(void* data); /* arg == NULL makes node with no data payload */

/* list foo */
list* new_list(node* n); /* arg == NULL make empty list */
int list_count(list* l); /* number of nodes in the list */
void list_add(list* l, node* n); /* adds non NULL node to end of list */
void list_remove(list* l, node* n); /* remove node anywhere in list */

/* pretend list is indexed */
node* list_getn(list* l, int index); /* get n'th node in list  */

/* Stack functions. Current pointer is NOT changed. */
void list_push(list* l, node* n); /* add new non NULL head node */
node* list_pop(list* l); /* return head and remove from list */

/* Iterator functions. The all use and/or modify the current pointer */
node* list_head(list* l); /* set current pointer to head and return it */
void list_rewind(list* l); /* set current pointer to head */
void list_seektoend(list* l); /* set current pointer to tail */
node* list_next(list* l); /* return next node */
node* list_current(list* l); /* return node pointed to by list->current */
void list_set_current(list* l, node* n); /* sets current to n */
node* list_tail(list* l); /* return last node */

#endif
