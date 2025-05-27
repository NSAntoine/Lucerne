//
//  resume_suspend_subcommands.c
//  lucerne
//
//  Created by Antoine on 27/05/2025.
//  

#include "resume_suspend_subcommands.h"
#include "target.h"

void suspend_subcommand(int argc, char **argv) {
    if (get_connected_target()) {
        task_suspend(get_connected_target()->task);
    }
}

void resume_subcommand(int argc, char **argv) {
    if (get_connected_target()) {
        task_resume(get_connected_target()->task);
    }
}
