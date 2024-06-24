#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

struct my_struct {
  int data;
  struct list_head list;
};

LIST_HEAD(my_list);

int example_init(void) {
  struct my_struct *n;
  n = kmalloc(sizeof(struct my_struct), GFP_KERNEL);
  n->data = 1;
  /* 본 코드의 핵심 함수 */
  list_add_tail(&n->list, &my_list);

  return 0;
}

void example_exit(void) {
  return;
}

module_init(example_init);
module_exit(example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("add node to the tail");
MODULE_AUTHOR("OS2019");

