//
//  subcommand.h
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#ifndef subcommand_h
#define subcommand_h

#include "memory_subcommand.h"
#include "target_subcommand.h"
#include "clear_subcommand.h"
#include "resume_suspend_subcommands.h"
#include "register_subcommand.h"

typedef void (*lucerne_command_handler)(int argc, char **argv);

#define MAX_ARGS 10

// Command which can have nested subcommands
typedef struct lucerne_command {
    const char *name;
    lucerne_command_handler handler;
    struct lucerne_command *subcommands;
} lucerne_command;

int tokenize(char *input, char **argv);
void parse_command(lucerne_command *cmds, int argc, char **argv);

#endif /* subcommand_h */
