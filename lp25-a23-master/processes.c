#include "processes.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdio.h>
#include <messages.h>
#include <file-properties.h>
#include <sync.h>
#include <string.h>
#include <errno.h>

/*!
 * @brief prepare prepares (only when parallel is enabled) the processes used for the synchronization.
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the program processes context
 * @return 0 if all went good, -1 else
 */
int prepare(configuration_t *the_config, process_context_t *p_context) {
    if (the_config->is_parallel == 0){
        return -1;
    }
    p_context->processes_count = 2;
    return 0;
}

/*!
 * @brief make_process creates a process and returns its PID to the parent
 * @param p_context is a pointer to the processes context
 * @param func is the function executed by the new process
 * @param parameters is a pointer to the parameters of func
 * @return the PID of the child process (it never returns in the child process)
 */
int make_process(process_context_t *p_context, process_loop_t func, void *parameters) {
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        func(parameters);
    } else {
        p_context->main_process_pid = pid;
        return pid;
    }
}

/*!
 * @brief lister_process_loop is the lister process function (@see make_process)
 * @param parameters is a pointer to its parameters, to be cast to a lister_configuration_t
 */
void lister_process_loop(void *parameters) {
}

/*!
 * @brief analyzer_process_loop is the analyzer process function
 * @param parameters is a pointer to its parameters, to be cast to an analyzer_configuration_t
 */
void analyzer_process_loop(void *parameters) {
}

/*!
 * @brief clean_processes cleans the processes by sending them a terminate command and waiting to the confirmation
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the processes context
 */
void clean_processes(configuration_t *the_config, process_context_t *p_context) {
    // Do nothing if not parallel
    if (!the_config->is_parallel) {
        return;
    }
    // Send terminate
    if (p_context->main_process_pid == getpid()) {
        // Send terminate signal to listers and analyzers
        for (int i = 0; i < p_context->processes_count; ++i) {
            kill(p_context->source_lister_pid, SIGINT);
            kill(p_context->destination_lister_pid, SIGINT);
            kill(p_context->source_analyzers_pids[i], SIGINT);
            kill(p_context->destination_analyzers_pids[i], SIGINT);
        }
    }

    // Wait for responses
    waitpid(p_context->source_lister_pid, NULL, 0);
    waitpid(p_context->destination_lister_pid, NULL, 0);
    for (int i = 0; i < p_context->processes_count; ++i) {
        waitpid(p_context->source_analyzers_pids[i], NULL, 0);
        waitpid(p_context->destination_analyzers_pids[i], NULL, 0);
    }
    
    // Free allocated memory
     free(p_context->source_analyzers_pids);
     free(p_context->destination_analyzers_pids);

    // Free the MQ
    msgctl(p_context->message_queue_id, IPC_RMID, NULL);
}

