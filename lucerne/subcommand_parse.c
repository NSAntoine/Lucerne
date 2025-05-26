//
//  subcommand_parse.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#include <stdio.h>
#include <string.h>
#include "subcommand.h"

int tokenize(char *input, char **argv) {
    int argc = 0;
    char *token = strtok(input, " ");
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    return argc;
}

// recursive parsing for commands that allows nested subcommands
// ie, the memory subcommand has the read & write subcommands
void parse_command(lucerne_command *cmds, int argc, char **argv) {
    if (argc == 0) return;
    
    for (int i = 0; cmds[i].name != NULL; ++i) {
        if (strcmp(argv[0], cmds[i].name) == 0) {
            if (cmds[i].subcommands && argc > 1) {
                parse_command(cmds[i].subcommands, argc - 1, argv + 1);
            } else if (cmds[i].handler) {
                cmds[i].handler(argc, argv);
            } else {
                printf("Unknown subcommand: %s\n", argv[0]);
            }
            return;
        }
    }
    printf("Unknown command: %s\n", argv[0]);
}
