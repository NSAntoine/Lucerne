//
//  backtrace_subcommand.c
//  lucerne
//
//  Created by Antoine on 16/07/2026.
//

#include <stdio.h>
#include "backtrace_subcommand.h"
#include "target.h"

typedef struct darwin_stack_frame {
    uint64_t previous_fp;
    uint64_t return_address;
} darwin_stack_frame_t;

kern_return_t safe_read_frame(lucerne_target *target, uint64_t fpaddr, darwin_stack_frame_t *out) {
    vm_size_t size;
    return vm_read_overwrite(target->task,
                             (vm_address_t)fpaddr,
                             sizeof(darwin_stack_frame_t),
                             (vm_address_t)out,
                             &size);
}

void backtrace_subcommand(int argc, char **argv) {
    lucerne_target *target = get_connected_target();
    if (!target) {
        printf("No target connected.\n");
        return;
    }

    mach_msg_type_number_t n_threads;
    thread_act_array_t threads;
    kern_return_t kr = task_threads(target->task, &threads, &n_threads);
    if (kr != KERN_SUCCESS || n_threads < 1) {
        printf("no threads??\n");
        return;
    }

    // TODO: - Determine which thread to choose, rather than just threads[0] lol
    arm_thread_state64_t state;
    mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;

    kr = thread_get_state(threads[0], ARM_THREAD_STATE64, (thread_state_t)&state, &count);
    if (kr != KERN_SUCCESS) {
        printf("failed to get thread state (kr = %d)\n", kr);
    }

    int frame_index = 0;
    
    uint64_t pc = arm_thread_state64_get_pc(state);
    uint64_t lr = arm_thread_state64_get_lr(state);
    uint64_t fp = arm_thread_state64_get_fp(state);
    printf("frame %d: pc 0x%016llx\n", frame_index++, pc);
    printf("frame %d: pc 0x%016llx\n", frame_index++, lr);
    darwin_stack_frame_t current_frame;

    while (fp) {
        kr = safe_read_frame(target, fp, &current_frame);
        if (kr != KERN_SUCCESS) {
            printf("failed to read frame at %llx (kr = %d)\n", fp, kr);
            break;
        }

        if (current_frame.return_address == 0) {
            break;
        }

        printf("frame %d: pc 0x%016llx\n", frame_index++, current_frame.return_address);

        // frame pointers should walk toward higher addresses on a sane stack
        if (current_frame.previous_fp == 0 || current_frame.previous_fp <= fp) {
            break;
        }

        fp = current_frame.previous_fp;
    }
}
