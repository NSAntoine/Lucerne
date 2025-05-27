//
//  memory_subcommand.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#include "memory_subcommand.h"
#include <stdio.h>
#include <stdlib.h>
#include "target.h"
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>

void print_lldb_style(mach_vm_address_t base_address, unsigned char *data, size_t size) {
    size_t i;
    for (i = 0; i < size; i += 16) {
        printf("0x%016llx: ", base_address + i);

        for (int j = 0; j < 16; j++) {
            if (i + j < size) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   "); // Padding for incomplete lines
            }
        }

        // Print ASCII
        printf(" ");
        for (int j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = data[i + j];
            printf("%c", isprint(c) ? c : '.');
        }

        printf("\n");
    }
}

// Read some address in memory. implemented with mach_vm_read
void memory_read_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("No target connected.\n");
        return;
    }
    
    if (argc < 2) {
        printf("usage: memory read <address> <size>\n");
        return;
    }
    
    char *addrString = argv[1];
    long addrNumber = strtol(addrString, NULL, 16);
    int size = 16;
    
    if (argc > 2) {
        size = atoi(argv[2]);
    }
    
    if (size < 0 || !(size % 16 == 0)) {
        printf("number of bytes must be positive and a multiple of 16.\n");
        return;
    }
    
    vm_offset_t data;
    mach_msg_type_number_t count;
    kern_return_t kr = mach_vm_read(target->task, addrNumber, size, &data, &count);
    
    if (kr != KERN_SUCCESS) {
        printf("Couldn't read memory (%s)\n", mach_error_string(kr));
        return;
    }
    
    if (count >= size) {
        print_lldb_style(addrNumber, (unsigned char *)data, count);
    } else {
        printf("Not enough data read.\n");
    }
}

// note: crashes when u try to overwrite strings? lol
void memory_write_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("No target connected.\n");
        return;
    }
    
    if (argc < 4) {
        printf("format: memory write <address> --value <HEX-VALUE> OR --file <file-path>\n");
        return;
    }
    
    int c;
    
    long hex_val = 0;
    bool hex_val_set = false;
    char *filename = NULL;
    
    while (1) {
        static struct option long_options[] =
        {
            {"value", required_argument, 0, 'v'},
            {"file",  required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        
        c = getopt_long (argc, argv, "v:f:",
                         long_options, &option_index);
        
        if (c == -1) {
            break;
        }
        
        switch (c) {
            case 'v': {
                char *endptr = NULL;
                hex_val = strtol(optarg, &endptr, 16);
                // validate hex input
                if (*endptr != '\0') {
                    printf("invalid hex value: %s\n", optarg);
                    return;
                }
                
                hex_val_set = true;
                break;
            }
            case 'f': {
                filename = strdup(optarg);
                break;
            }
            default:
                printf("invalid options\n");
                return;
        }
    }
    
    optind = 1; // reset getopt
    
    long addrNumber = strtol(argv[3], NULL, 16); // getopt reorders the address to be last LOL
    
    kern_return_t kr;
    
    if (hex_val_set) {
        mach_vm_protect(target->task, addrNumber, sizeof(long), FALSE, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY);
        kr = mach_vm_write(target->task, addrNumber, (vm_offset_t)&hex_val,  (mach_msg_type_number_t)sizeof(long));
    } else if (filename) {
        
        // open file, read data, write data
        FILE *fptr = fopen(filename, "rb");
        if (!fptr) {
            printf("couldn't open file: %s\n", strerror(errno));
            return;
        }
        
        // seek to the end so we can get the size of the file
        fseek(fptr, 0, SEEK_END);
        long filelen = ftell(fptr);
        rewind(fptr); // jump back to beginning so we can get contents properly
        
        char *buffer = malloc(filelen * sizeof(char));
        fread(buffer, filelen, 1, fptr);
        
        mach_vm_protect(target->task, addrNumber, (mach_msg_type_number_t)filelen, FALSE, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY);
        kr = mach_vm_write(target->task, addrNumber, (pointer_t)buffer, (mach_msg_type_number_t)filelen);
    } else {
        printf("Please specify a hex value with --value or a file with --file\n");
        return;
    }
    
    if (kr != KERN_SUCCESS) {
        printf("Error writing memory: %s\n", mach_error_string(kr));
    } else {
        printf("Wrote to memory.\n");
    }
}

void memory_subcommand(int argc, char **argv) {
    printf("Please use one of the memory subcommands: read, write.\n");
}
