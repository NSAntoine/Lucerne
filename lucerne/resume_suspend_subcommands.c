//
//  resume_suspend_subcommands.c
//  lucerne
//
//  Created by Antoine on 27/05/2025.
//  

#include "resume_suspend_subcommands.h"
#include "target.h"
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

void suspend_subcommand(int argc, char **argv) {
    if (get_connected_target()) {
        task_suspend(get_connected_target()->task);
    }
}

void resume_subcommand(int argc, char **argv) {
     if (get_connected_target()) {
         task_resume(get_connected_target()->task);
         
         thread_act_array_t threads;
         mach_msg_type_number_t thread_n;
         task_threads(get_connected_target()->task, &threads, &thread_n);
         for (int i = 0; i < thread_n; i++) {
             thread_resume(threads[i]);
         }
    }
}
