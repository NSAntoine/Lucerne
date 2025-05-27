//
//  target.c
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#include "target.h"
#include <mach/mach.h>
#include <sys/_types/_pid_t.h>
#include <libproc.h>
#include <stdlib.h>
#include <stdio.h>
#include <dispatch/dispatch.h>

static lucerne_target *connected_target = NULL;
static dispatch_queue_t target_death_observer_dispatch_queue;
static struct kevent target_death_kevent_observer;
static int target_death_kqueue;

lucerne_target *get_connected_target(void) {
    return connected_target;
}

void set_connected_target(lucerne_target *target) {
    connected_target = target;
    
    if (!target_death_observer_dispatch_queue)
        target_death_observer_dispatch_queue = dispatch_queue_create("com.antoine.lucerne.target_death_observer", NULL);
    
    // watch out for when the target dies,
    // and disconnect when/if it does
    dispatch_async(target_death_observer_dispatch_queue, ^{
        target_death_kqueue = kqueue();
        EV_SET(&target_death_kevent_observer, target->pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
        
        // this is blocking (which is why we run it on an async q)
        int nev = kevent(target_death_kqueue, &target_death_kevent_observer, 1, &target_death_kevent_observer, 1, NULL);
        
        if (nev == -1) {
            perror("kevent :(");
        } else if (nev > 0) {
            if (target_death_kevent_observer.fflags & NOTE_EXIT) {
                printf("\nProcess %d has exited. Disconnecting.\n", target->pid);
                disconnect_target();
            }
        }
    });
}

void disconnect_target(void) {
    if (get_connected_target()) {
        EV_SET(&target_death_kevent_observer, get_connected_target()->pid, EVFILT_PROC, EV_DELETE, 0, 0, NULL);
        kevent(target_death_kqueue, &target_death_kevent_observer, 1, NULL, 0, NULL);
        
        close(target_death_kqueue);
        
        free(connected_target);
        connected_target = NULL;
    }
}

lucerne_init_target_result
lc_init_target_from_pid(pid_t pid, lucerne_target **target) {
    // first, let's get task_for_pid, this is mainly what we care about
    // (the name property is more so just for show if needed)
    task_t tsk;
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &tsk); /* the secret ingredient */
    
    if (kr != KERN_SUCCESS) {
        // failed :(
        target = NULL;
        return (lucerne_init_target_result){
            .code = lucerne_init_target_kr_err,
            .kr_error = kr
        };
    }
    
    char *name = malloc(2*MAXCOMLEN);
    proc_name(pid, name, 2*MAXCOMLEN);
    
    *target = malloc(sizeof(lucerne_target));
    (*target)->name = strdup(name);
    (*target)->pid = pid;
    (*target)->task = tsk;
    
    return (lucerne_init_target_result) {
        .code = lucerne_init_target_success,
        .kr_error = -1
    };
};

lucerne_init_target_result
lc_init_target_from_name(const char *user_input_name, lucerne_target **target) {
    
    // get all pid names, iterate, and find the ones that match the name
    // TODO: - Change pids to be allocated depending on how many pids actually exist with malloc
    // (and not just hardcoding 1024)
    pid_t pids[1024];
    int count = proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
    
    // There may be multiple processes with the same name,
    // so lets a store an array of indices (of the pid array)
    // to later give the user choice of which they want to select
    int indices[200] = {0};
    unsigned int indicesSize = 0; // amount of indices actually in the array
    
    for (int i = 0; i < count; i++) {
        char process_name[2*MAXCOMLEN];
        proc_name(pids[i], process_name, sizeof(process_name));
        
        if (strcmp(user_input_name, process_name) == 0) {
            // insert index
            indices[indicesSize] = i;
            indicesSize++;
        }
    }
    
    switch (indicesSize) {
        case 0: // no procs match the name :(
            printf("here\n");
            return (lucerne_init_target_result) {
                .code = lucerne_init_target_not_found,
                .kr_error = -1
            };
        case 1: { // ideal case: exactly one proc matches that name, we can connect right away
            int index = indices[0];
            printf("found proc at pid %d\n", pids[index]);
            return lc_init_target_from_pid(pids[index], target);
        }
        default: { // >1, give the user a choice
            // TODO: - Remove user choice, just show possible PIDs and tell the user to specify which one in another connect command
            pid_t *user_choices = malloc(indicesSize*sizeof(pid_t));
            for (int i = 0; i < indicesSize; i++) {
                user_choices[i] = pids[indices[i]]; // wow this SUCKS LOL
            }
            
            return (lucerne_init_target_result) {
                .code = lucerne_init_target_multiple_found,
                .kr_error = -1,
                .choices = user_choices,
                .choices_count = indicesSize
            };
        }
    }
}
