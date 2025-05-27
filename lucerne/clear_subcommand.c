//
//  clear_subcommand.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#include "clear_subcommand.h"
#include <curses.h>

void clear_subcommand(int argc, char **argv) {
    printf("\e[1;1H\e[2J");
}
