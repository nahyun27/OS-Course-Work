#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

struct birthday {
  int day;
  int month;
  int year;
  struct list_head list;
};

static LIST_HEAD(birthday_list);

struct birthday *createBirthday(int day, int month, int year) {

  /* TODO: 생일을 위한 메모리를 할당하고, 인자들을 채워 생일을 완성하세요. */
  struct birthday *temp = kmalloc(sizeof(struct birthday), GFP_KERNEL);

  temp->day = day;
  temp->month = month;
  temp->year = year;
  
  INIT_LIST_HEAD(&temp->list);

  return temp;

}

int simple_init(void) {
  printk("INSTALL MODULE: bdlist\n");

  /* TODO: 생일 목록을 하나씩 생성하는대로 연결리스트에 연결시키세요(노드 삽입). */
  struct birthday *temp1 = createBirthday(27, 6, 2001);
  struct birthday *temp2 = createBirthday(21, 3, 2000);
  list_add_tail(&temp1->list, &birthday_list);
  list_add_tail(&temp2->list, &birthday_list);
    
  /* TODO: 완성된 연결리스트를 탐색하는 커널 함수를 사용하여 출력하세요. */
  struct birthday *cur;
  list_for_each_entry(cur,&birthday_list,list){
      printk("Day %d.%d.%d\n", cur->day, cur->month, cur->year);
  }
  return 0;
}

void simple_exit(void) {
  /* 모듈을 제거할 때는 생성한 연결 리스트도 하나씩 제거하며 끝내도록 하세요. */
  /* 제거를 하기 전에 리스트가 "비어있을 경우""에 대한 예외처리를 하는게 좋겠죠? */
  if(list_empty(&birthday_list)) {
    printk("List is Empty\n");
    return;
  }

  /* TODO: 이제 본격적으로 연결리스트를 탐색하면서 하나씩 제거하도록 하시면 됩니다. */
  struct birthday *birthday;
  struct list_head *cur, *next;
  
  list_for_each_safe(cur,next,&birthday_list){
      birthday = list_entry(cur, struct birthday, list);
      printk("Remove %d.%d.%d\n",birthday->day,birthday->month,birthday->year);
      list_del(cur);
      kfree(cur);
  }
  printk("REMOVE MODULE: bdlist\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("make a list of birthdays and print");
MODULE_AUTHOR("이름_학번");
