#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

bool lazy_load_segment (struct page *page, void *aux);

/* --------------- Project 3 ---------------- */
// static bool install_page (void *upage, void *kpage, bool writable);
// bool lazy_load_segment (struct page *page, void *aux);

/* Lazy load를 위한 정보 구조체 */
/* Page에 대응되는 File의 정보들이 들어감 */
struct container {
    struct file *file;
    off_t offset; // 해당 File의 Offset
    size_t page_read_bytes; // 읽어놀 File의 데이터 크기(load_segment에서 1 Page보다 작거나 같다.)
};
/* ------------------------------------------ */

#endif /* userprog/process.h */
