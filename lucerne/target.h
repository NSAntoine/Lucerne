//
//  target.h
//  lucerne
//
//  Created by Antoine on 26/05/2025.
//  

#ifndef target_h
#define target_h

#include <mach/mach.h>
#include <sys/_types/_pid_t.h>
#include <stdbool.h>

typedef struct {
    thread_act_t thread_act;
} lucerne_target_thread;

typedef struct {
    
    task_t task;
    
    uint64_t selected_thread_id;
    
//    exception_mask_t       saved_masks[EXC_TYPES_COUNT];
//    mach_port_t            saved_ports[EXC_TYPES_COUNT];
//    exception_behavior_t   saved_behaviors[EXC_TYPES_COUNT];
//    thread_state_flavor_t  saved_flavors[EXC_TYPES_COUNT];
//    mach_msg_type_number_t saved_exception_types_count;
} lucerne_target;

pid_t pid_for_target(lucerne_target *target);

typedef enum {
    lucerne_init_target_success,
    lucerne_init_target_not_found,
    lucerne_init_target_kr_err,
    lucerne_init_target_multiple_found /* occurs if the user enters a proc name but multiple processes are found with that name */
} lucerne_init_target_result_code;

typedef struct {
    lucerne_init_target_result_code code;
    
    /* if an error occured from the kernel side, this stores it. -1 means no error */
    kern_return_t kr_error;
    
    /* if the user entered a name of a process and multiple processes matched that name, we give them the choice to decide */
    pid_t *choices;
    
    int choices_count;
} lucerne_init_target_result;

lucerne_init_target_result
lc_init_target_from_pid(pid_t pid, lucerne_target **target);

lucerne_init_target_result
lc_init_target_from_name(const char *name, lucerne_target **target);

lucerne_target *get_connected_target(void);
bool set_connected_target(lucerne_target *);
void disconnect_target(void);

#endif /* target_h */
