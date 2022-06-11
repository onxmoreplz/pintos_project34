#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
bool install_page(void *upage, void *kpage, bool writable);
 

/* Project 3 */
/* 페이지 폴트가 발생한 후 Disk에서 필요한 파일을 불러올 때 필요한 파일 정보를 알 수 있다. */
struct file_info {
	struct file *file;
	off_t ofs;
	size_t page_read_bytes; // 읽어올 파일의 데이터 크기(load_segment()에서 1 PAGE보다 작거나 같다)
	bool writable;
};

#endif /* userprog/process.h */
