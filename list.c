/* 
Copyright 2020 Caravan Electronic Publishing, LLC

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



*/

/* this is an implementation of a doubly-linked list with the 
   addition of stack and iterator like features. The nodes
   contain void* to opaque data. Since node data is opaque,
   use of free() is left to the user.
*/

#include "list.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* 
   node functions
*/

/* constructor */
node* new_node(void* data)
{
  node* n;

  if(data==NULL){
    return(NULL);
  }
  
  if((n=malloc(sizeof(node))) == NULL){
    perror("new_node():");
    exit(1);
  }

  n->next=NULL;
  n->prev=NULL;
  n->data=data;

  return(n);
}


/* 
   List functions, e.g. create list, add nodes, etc... 
*/

list* new_list(node* n)
{
  list* l;

  if((l=malloc(sizeof(list))) == NULL){
    perror("new_list():");
    exit(1);
  }

  l->current=NULL; /* current needs an initial value, but we're not 
		    "setting" it to anything, including node n */

  l->head=n;
  l->tail=n;

  if(n == NULL){
    l->count=0;
  }else{
    l->count=1;
  }
  
  return(l);
}


int list_count(list* l)
{
  return(l->count);
}


/* add a node to the end of the list */
void list_add(list* l, node* n)
{
  if(n==NULL){
    /* do nothing if node is NULL */
  }else if(l->count == 0){
    l->head=n;
    l->tail=n;
  }else{
    n->prev=l->tail;
    n->next=NULL;
    l->tail->next=n;
    l->tail=n;
  }
  l->count += 1;
}


/* assume user knows what they are doing when passing n */
void list_remove(list* l, node* n)
{
  /* lists can be read in either direction, and iterators 
     need to behave sensibly, so deleting the current node
     leads to an undefined state and it's best to delete the 
     node and mark the current pointer as NULL */
  switch(l->count){
  case 0:
    /* we should not get here */
    break;    
  case 1:
    l->head=NULL;
    l->tail=NULL;
    l->count=0;
    break;
  default:
    if(n == l->tail){ /* at end of list */
      l->tail=n->prev;
      l->tail->next=NULL;
    }else if(n == l->head){ /* at beginning of list */
      l->head=n->next;
      l->head->prev=NULL;
    }else{ /* somewhere in between */
      n->prev->next=n->next;
      n->next->prev=n->prev;
    }
    n->next=NULL;
    n->prev=NULL;
    l->count-=1;
    break;
  }
}


/*
  Pretend list is a stack. 
*/

void list_push(list* l, node* n) 
{
  if(n==NULL){
    /* do nothing if node is NULL */
  }else if(l->count == 0){
    list_add(l, n);
  }else{
    n->next=l->head;
    n->prev=NULL; /* we initialize this as NULL earlier too */
    l->head->prev=n;
    l->head=n;
    l->count += 1;
  }
}


node* list_pop(list* l)
{
  node* n;
  
  if(l->count == 0){
    return(NULL);
  }
  
  n=l->head;
  list_remove(l, n);
  return(n);
}


/* Pretend list is an indexed array. Node numbering start at 0,
   with 0 being the head node */
node* list_getn(list* l, int index)
{
  int i;
  node* n;

  if( l->count == 0 || index < 0 || index > (l->count - 1) ){
    return(NULL);
  }else if(index == (l->count - 1)){ /* special case can be more efficient */
    return(l->tail);
  }

  if(index <= (l->count / 2)){ /* start from front */
    n=l->head;
    for(i=0; i < index; ++i ){
      n=n->next;
    }
    return(n);
  }else{ /* start from end */
    n=l->tail;
    for(i=(l->count - 1); i > index; --i){
      n=n->prev;
    }
    return(n);
  }
}


node* list_head(list* l)
{
  return(l->current=l->head);
}


void list_seektoend(list* l)
{
  l->current=l->tail;
}


node* list_prev(list* l)
{
   if(l->count == 0 || l->current == l->head){
    return(NULL);
   }
   return(l->current=l->current->prev);
}


node* list_next(list* l)
{
  if(l->count == 0 || l->current == l->tail){
    return(NULL);
  }
  return((l->current=l->current->next));

}


node* list_current(list* l)
{
  return(l->current);
}

/* assumes user know what they are doing and DOES NOT 
search the list to see if n is a member */
void list_set_current(list* l, node* n)
{
  l->current=n;
}

node* list_tail(list* l)
{
  return(l->current=l->tail);
}




  

#ifdef DEBUG_LIST_C

int main(int argc, char* argv[])
{
  list* l;
  node* n;

  n=new_node("hello world");
  l=new_list(n);

  printf("list_pop(l)->data=%s\n", (char *)(list_pop(l)->data));
  printf("now list_count(l) is %d\n", list_count(l));

  list_add(l, new_node("hello again"));
  printf("adding new node\nlist_head(l)->data=%s\n", (char *)(list_head(l)->data));

  list_add(l, new_node("hello again 2"));
  printf("adding another node\nlist_tail(l)->data=%s\n", (char *)(list_tail(l)->data));

  printf("node_getn(l, 2)=%s\n", (char *)(list_getn(l,1)->data));
  return(0);
}

#endif
