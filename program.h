#ifndef PROGRAM_H
#define PROGRAM_H

/* parent process */
void process_1();
/* child process */
void process_2();
/* signal handler */
void handler(int sig);
/* prints signals come to process 1 in critical section*/
void print_signal_info();
/* creates file descriptors for files */
void create_fd();
/* close files */
void close_files_process_1();

#endif