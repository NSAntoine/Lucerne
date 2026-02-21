//
//  register_subcommand.c
//  lucerne
//
//  Created by Antoine on 27/05/2025.
//  

#include "register_subcommand.h"
#include "target.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    uint64_t value;
} lucerne_register_key_value_pair;

typedef enum {
    lucerne_x_general_register,
    lucerne_w_general_register,
    lucerne_r_general_register,
    lucerne_not_a_general_purpose_register
} lucerne_register_type;

#define ARM_SPECIAL_REGISTERS_COUNT (5)

void register_subcommand(int argc, char **argv) {
    printf("Please use one of the register subcommands: read, write\n");
}

void read_specific_register(const char *register_name, lucerne_target *target);

// TODO: - Add x86 support
// TODO: - for various commands, require that the process be suspended
void register_read_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("No target connected.\n");
        return;
    }
    
    // register name specified: print that specific registers
    if (argc >= 2) {
        char *register_name = argv[1];
        read_specific_register(register_name, target);
    } else {
        // print all registers
        mach_msg_type_number_t n_threads;
        thread_act_array_t threads;
        task_threads(target->task, &threads, &n_threads);
        
        // TODO: - Determine which thread to choose, rather than just threads[0] lol
        if (n_threads < 1) {
            printf("no threads??\n");
            return;
        }
        
        arm_thread_state64_t state;
        mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
        
        kern_return_t kr = thread_get_state(threads[0], ARM_THREAD_STATE64, (thread_state_t)&state, &count);
        if (kr != KERN_SUCCESS) {
            printf("Error reading registers (thread_get_state): %s\n", mach_error_string(kr));
            return;
        }
        
        for (int i = 0; i < 29; i++) {
            printf("\t%sx%d = 0x%016llx\n", i >= 10 ? "" : " ", i, state.__x[i]);
        }
        
        printf("\t fp = 0x%016llx\n", state.__fp);
        printf("\t lr = 0x%016llx\n", state.__lr);
        printf("\t sp = 0x%016llx\n", state.__sp);
        printf("\t pc = 0x%016llx\n", state.__pc);
        printf("\t cpsr = 0x%08x\n", state.__cpsr);
//        lucerne_register_key_value_pair pairs[ARM_SPECIAL_REGISTERS_COUNT] = {
//            {"fp", state.__fp},
//            {"lr", state.__lr},
//            {"sp", state.__sp},
//            {"pc", state.__pc},
//            {"cpsr", state.__cpsr}
//        };
    }
}

void parse_register_type_and_number(const char *reg_string,
                                    lucerne_register_type *reg_type_out,
                                    int *reg_number) {
    if (strlen(reg_string) >= 2 && (reg_string[1] >= '0' && reg_string[1] <= '9')) {
        switch (reg_string[0]) {
            case 'x':
                *reg_type_out = lucerne_x_general_register;
                break;
            case 'w':
                *reg_type_out = lucerne_w_general_register;
                break;
            case 'r':
                *reg_type_out = lucerne_r_general_register;
                break;
            default:
                *reg_type_out = lucerne_not_a_general_purpose_register;
                break;
        }
        
        if (*reg_type_out != lucerne_not_a_general_purpose_register) {
            // do &reg_string[1] to skip over the first element (ie, the x in x5)
            *reg_number = atoi(&reg_string[1]);
        }
    } else {
        *reg_type_out = lucerne_not_a_general_purpose_register;
        *reg_number = -1;
    }
}

void read_specific_register(const char *register_name, lucerne_target *target) {
    
    mach_msg_type_number_t n_threads;
    thread_act_array_t threads;
    task_threads(target->task, &threads, &n_threads);
    
    int register_number = -1; // if it's -1, it's not a general purpose regsiter
    
    lucerne_register_type register_type = lucerne_not_a_general_purpose_register;
    
    // if the string is formatted like
    // x0, r3, w4, etc... then it's a general purpose register
    // and we must request the thread state as either 64 bit or 32 bit
    // so let's check if it's a 32bit general purpose register
    // to determine if we want the 32bit state
    if (strlen(register_name) >= 2 && (register_name[1] >= '0' && register_name[1] <= '9')) {
        switch (register_name[0]) {
            case 'x':
                register_type = lucerne_x_general_register;
                break;
            case 'w':
                register_type = lucerne_w_general_register;
                break;
            case 'r':
                register_type = lucerne_r_general_register;
                break;
            default:
                break;
        }
        
        if (register_type != lucerne_not_a_general_purpose_register)
            register_number = atoi(&register_name[1]);
    }
    
    // TODO: - Determine which thread to choose, rather than just threads[0] lol
    if (n_threads < 1) {
        printf("no threads??\n");
        return;
    }
    
    // 32bit general purpose register branch
    if (register_type == lucerne_r_general_register) {
        arm_thread_state_t state;
        mach_msg_type_number_t count = ARM_THREAD_STATE_COUNT;
        
        kern_return_t kr = thread_get_state(threads[0], ARM_THREAD_STATE, (thread_state_t)&state, &count);
        if (kr != KERN_SUCCESS) {
            printf("Error reading registers (thread_get_state): %s\n", mach_error_string(kr));
            return;
        }
        
        // register_number can't be -1 here, so if we're here it's for a general purpose register
        printf("%s: %d\n", register_name, state.__r[register_number]);
        
        return;
    }
    
    arm_thread_state64_t state;
    mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
    
    kern_return_t kr = thread_get_state(threads[0], ARM_THREAD_STATE64, (thread_state_t)&state, &count);
    if (kr != KERN_SUCCESS) {
        printf("Error reading registers (thread_get_state): %s\n", mach_error_string(kr));
        return;
    }
    
    switch (register_type) {
        case lucerne_w_general_register:
            printf("\t%s = %8x\n", register_name, (uint32_t)state.__x[register_number]);
            break;
        case lucerne_x_general_register:
            printf("\t%s = 0x%016llx\n", register_name, state.__x[register_number]);
            break;
        case lucerne_r_general_register:
            printf("Shouldn't be here\n");
            break;
        case lucerne_not_a_general_purpose_register: {
            lucerne_register_key_value_pair pairs[ARM_SPECIAL_REGISTERS_COUNT] = {
                {"fp", state.__fp},
                {"lr", state.__lr},
                {"sp", state.__sp},
                {"pc", state.__pc},
                {"cpsr", state.__cpsr}
            };
            
            bool found = false;
            for (int i = 0; i < ARM_SPECIAL_REGISTERS_COUNT; i++) {
                if (strcmp(pairs[i].name, register_name) == 0) {
                    printf("\t%s = 0x%016llx\n", register_name, pairs[i].value);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                printf("Unrecognized register \"%s\"\n", register_name);
            }
            
            break;
        }
    }
}

// TODO: - Add x86 support too
void register_write_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("No target connected.\n");
        return;
    }
    
    if (argc < 3) {
        printf("format: register write <register> <value>");
    }
    
    char *register_name = argv[1];
    char *value_char = argv[2];
    
    lucerne_register_type reg_type;
    int reg_number;
    parse_register_type_and_number(register_name, &reg_type, &reg_number);
    
    char *endptr;
    long value = strtol(value_char, &endptr, 16);
    if (*endptr != '\0') {
        printf("value must be a valid hexadecimal number.");
        return;
    }
    
    // TODO: - AGAIN... determine which thread to actually use... rather than threads[0]
    mach_msg_type_number_t n_threads;
    thread_act_array_t threads;
    // TODO: - Validate task_threads return
    task_threads(target->task, &threads, &n_threads);
    
    if (n_threads < 1) {
        printf("No threads??\n");
        return;
    }
    
    // TODO: - 32bit ARM support (boring!)
    if (reg_type == lucerne_r_general_register) {
        return;
    }
    
    // get existing thread state
    arm_thread_state64_t state;
    mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
    kern_return_t get_state_kr = thread_get_state(threads[0], ARM_THREAD_STATE64,
                                                  (thread_state_t)&state, &count);
    if (get_state_kr != KERN_SUCCESS) {
        printf("Error trying to get thread state: %s\n", mach_error_string(get_state_kr));
        return;
    }
    
    //TODO: - Write to the register properly lol
    mach_msg_type_number_t set_state_count = ARM_THREAD_STATE64_COUNT;
    
    switch (reg_type) {
        case lucerne_w_general_register:
        case lucerne_x_general_register:
            state.__x[reg_number] = value;
            break;
        case lucerne_not_a_general_purpose_register: {
            // store the pairs but as pointers
            // probably evil... but it works hahahaha
            lucerne_register_key_value_pair pairs[ARM_SPECIAL_REGISTERS_COUNT] = {
                {"fp",   (uint64_t)&state.__fp},
                {"lr",   (uint64_t)&state.__lr},
                {"sp",   (uint64_t)&state.__sp},
                {"pc",   (uint64_t)&state.__pc},
                {"cpsr", (uint64_t)&state.__cpsr}
            };
            
            bool found = false;
            for (int i = 0; i < ARM_SPECIAL_REGISTERS_COUNT; i++) {
                if (strcmp(pairs[i].name, register_name) == 0) {
                    // cpsr is 32 bit soo make a special exception for it
                    if (strcmp("cpsr", register_name) == 0) {
                        *((uint32_t *)pairs[i].value) = (uint32_t)value;
                    } else {
                        *((int64_t *)pairs[i].value) = value;
                    }
                    
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                printf("No such register with name \"%s\"\n", register_name);
                return;
            }
        }
            break;
        default:
            break;
    }
    
    kern_return_t setStateKR = thread_set_state(threads[0],
                                                  ARM_THREAD_STATE64,
                                                  (thread_state_t)&state,
                                                  set_state_count);
    
    if (setStateKR != KERN_SUCCESS) {
        printf("Error trying to set state: %s\n", mach_error_string(setStateKR));
    } else {
        printf("Success?\n");
    }
}
