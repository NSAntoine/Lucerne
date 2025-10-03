//
//  target_subcommand.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//

#include "target_subcommand.h"
#include "target.h"

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <libproc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void handle_lc_init(lucerne_init_target_result res, lucerne_target **target) {
    switch (res.code) {
        case lucerne_init_target_success:
            if (!set_connected_target(*target)) {
                printf("Failed to connect to process.\n");
                free(*target);
                *target = NULL;
            }
            printf("Connected to process.\n");
            break;
        case lucerne_init_target_kr_err:
            printf("System Error trying to connect: %s\n", mach_error_string(res.kr_error));
            break;
        case lucerne_init_target_not_found:
            printf("Couldn't find process.\n");
            break;
        case lucerne_init_target_multiple_found:
            printf("Multiple processes with the same name found, please select one by typing the number inbetween brackets:\n");
            for (int i = 0; i < res.choices_count; i++) {
                char process_name[2*MAXCOMLEN];
                proc_name(res.choices[i], process_name, sizeof(process_name));
                printf("[%d] %s (pid: %d)\n", i+1, process_name, res.choices[i]);
            }
            
            char choice[5];
            fgets(choice, 5, stdin);
            printf("%s\n", choice);
            int n = atoi(choice);
            if (n < 1) {
                printf("Invalid input. Please try the connect command again.");
                return;
            }
            
            pid_t chosen_pid = res.choices[n - 1];
            free(res.choices);
            res = lc_init_target_from_pid(chosen_pid, target);
            handle_lc_init(res, target);
            break;
    }
}

void target_connect_subcommand(int argc, char **argv) {
    
    //    for (int i = 0; i < argc; i++) {
    //        printf("%s ", argv[i]);
    //    }
    
    pid_t pid = -1;
    char *name = NULL;
    
    int c;
    
    while (1) {
        static struct option long_options[] =
        {
            {"pid",  required_argument, 0, 'p'},
            {"name",  required_argument, 0, 'n'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        
        c = getopt_long (argc, argv, "p:n:",
                         long_options, &option_index);
        
        if (c == -1) {
            break;
        }
        
        switch (c) {
            case 'p':
                pid = atoi(optarg);
                break;
            case 'n':
                name = strdup(optarg);
                break;
            default:
                printf("invalid options\n");
                return;
        }
    }
    
    optind = 1; // reset getopt (yes this sucks ik!)
    
    lucerne_target *target = NULL;
    lucerne_init_target_result res;
    
    if ( (pid != -1 && pid != 0) ) {
        // pid is valid, use it
        res = lc_init_target_from_pid(pid, &target);
    } else if (name) {
        // name is valid, use it
        res = lc_init_target_from_name(name, &target);
        free(name);
    } else {
        printf("Please provide either a pid or name to connect to by --pid / --name\n");
        exit(EXIT_FAILURE);
    }
    
    handle_lc_init(res, &target);
}

void target_disconnect_subcommand(int argc, char **argv) {
    if (get_connected_target()) {
        disconnect_target();
        printf("Disconnected from target.\n");
    } else {
        printf("No target connected in the first place");
    }
}

void target_info_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("Not connected to a target.\n");
        return;
    }
    
    pid_t pid = pid_for_target(target);
    
    char *name = malloc(2*MAXCOMLEN);
    proc_name(pid, name, 2*MAXCOMLEN);
    
    printf("Target name: %s\nPID: %d\n", name, pid);
}

void target_subcommand(int argc, char **argv) {
    printf("Please use one of the target subcommands: connect, disconnect, info.\n");
}
