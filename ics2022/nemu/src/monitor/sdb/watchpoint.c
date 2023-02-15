/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  int ori_val;
  char* ori_expr;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// Th: One function just carry one role
// ~

WP* new_wp()
{
  if(!free_)  assert(0);
  WP* ret=free_;
  free_=free_->next;
  ret->next=head;
  head=ret;
  ret->ori_expr=(char *)malloc(25);
  return ret;
}

void free_wp(WP* wp)
{
  if(!wp || !head) assert(0);
  WP *tmp=head;
  // bug here
  if(tmp==wp) 
  {
    head=head->next;
  }
  else
  {
  while(tmp->next!=wp)  tmp=tmp->next;
  if(!tmp)  assert(0);
  }
  free(wp->ori_expr);
  tmp->next=wp->next;
  wp->next=free_;
  free_=wp;
  

}

// TODO: specific function operation
void set_wp(char* ori_str,word_t val)
{
  WP* tmp_wp=new_wp();
  // SEGV
  assert(tmp_wp->ori_expr);
  strcpy(tmp_wp->ori_expr,ori_str);
  tmp_wp->ori_val=val;
  printf("Have set a new watchpoint (ID = %d)\n",tmp_wp->NO);
}

void remove_wp(int id)
{
  assert(id < NR_WP);
  WP *wp=&wp_pool[id];
  free_wp(wp);
  printf("Have removed watchpoint (ID = %d)\n",wp->NO);
}

void show_wp(){
  WP *tmp_wp=head;
  if(!tmp_wp) puts("There doesn't exist any watchponit!");
  while(tmp_wp){
    printf("%d:%s \n",tmp_wp->NO,tmp_wp->ori_expr);
    tmp_wp=tmp_wp->next;
  }
}

void wp_difftest(){
  // Doing
  //puts("Difftest here");
  WP *tmp_wp=head;
  while(tmp_wp){
    bool flag;
    word_t new_val=expr(tmp_wp->ori_expr,&flag);
    //printf("new_val=%u\n",new_val);
    if(tmp_wp->ori_val!=new_val)
    {
      printf("Watchpoint %d has been triggered\n"
      
      ,tmp_wp->NO,tmp_wp->ori_val,new_val);
      tmp_wp->ori_val=new_val;
    }
    tmp_wp=tmp_wp->next;
  }
}