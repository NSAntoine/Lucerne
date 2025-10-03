//
//  main.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#include <stdio.h>
#include <mach/mach.h>
#include "subcommand.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include "target.h"

#define REPL_INPUT_SIZE 1024

lucerne_command target_subcmds[] = {
    {"connect", target_connect_subcommand, NULL},
    {"disconnect", target_disconnect_subcommand, NULL},
    {"info", target_info_subcommand, NULL},
    {NULL, NULL, NULL}
};

lucerne_command register_subcmds[] = {
    {"read", register_read_subcommand, NULL},
    {"write", register_write_subcommand, NULL},
    {NULL, NULL, NULL}
};

lucerne_command memory_subcommands[] = {
    {"read", memory_read_subcommand, NULL},
    {"write", memory_write_subcommand, NULL},
    {NULL, NULL, NULL}
};

lucerne_command commands[] = {
    {"target", target_subcommand, target_subcmds},
    {"memory", memory_subcommand, memory_subcommands},
    {"register", register_subcommand, register_subcmds},
    {"suspend", suspend_subcommand, NULL},
    {"resume", resume_subcommand, NULL},
    {"clear",  clear_subcommand,  NULL},
    {NULL, NULL, NULL}
};

int main(void) {
    
    while (1) {
        char *argv[MAX_ARGS];
        int argc;
        
        char response[REPL_INPUT_SIZE];
        fputs("Lucerne> ", stdout);
        fgets(response, REPL_INPUT_SIZE, stdin);
        
        // trim the dumbass newline
        response[strcspn(response, "\n")] = 0;
        argc = tokenize(response, argv);
        if (argc > 0) {
            parse_command(commands, argc, argv);
        }
    }
    
//    task_t nm;
//    kern_return_t kr = task_for_pid(mach_task_self(), 8506, &nm);
//    if (kr == KERN_SUCCESS) {
//        printf("success!\n");
//    } else {
//        printf("noo :( (%s)\n", mach_error_string(kr));
//    }
//    // insert code here...
//    vm_offset_t data;
//    mach_msg_type_number_t cnt;
//    
//    mach_vm_read(nm, 0x600000ca0020, sizeof(int), &data, &cnt);
//    int value;
//    if (cnt >= sizeof(int)) {
//        value = *(int *)data;
//        printf("Read int: %d (0x%x)\n", value, value);
//    } else {
//        printf("Not enough data read.\n");
//    }
    
    return 0;
}
