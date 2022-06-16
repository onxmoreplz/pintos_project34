#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
struct page;
enum vm_type;

struct anon_page {
    /* --------------- Project 3 --------------- */
    int swap_index; // Swap된 데이터들이 저장된 섹터 구역을 의미
    /* ----------------------------------------- */
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
