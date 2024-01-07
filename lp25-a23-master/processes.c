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
    if (the_config != NULL && the_config->is_parallel == true) {
        
        p_context->shared_key = ftok("lp25_backup", 11);
        
        if (p_context->shared_key == -1) {
            perror("ERROR with shared_key\n");
            return -1;
        }

    

        p_context->processes_count = 0;
        p_context->main_process_pid = getpid();

        p_context->message_queue_id = msgget(p_context->shared_key, 0666 | IPC_CREAT);
        if (p_context->message_queue_id == -1) {
            perror("ERROR with msgget!\n");
            return -1;
        }
    

        lister_configuration_t cfg_dest_lister;
        cfg_dest_lister.mq_key = p_context->shared_key;
        cfg_dest_lister.my_recipient_id = MSG_TYPE_TO_DESTINATION_ANALYZERS;
        cfg_dest_lister.my_receiver_id = MSG_TYPE_TO_DESTINATION_LISTER;
        cfg_dest_lister.analyzers_count = (the_config->processes_count-2)/2;
        p_context->destination_lister_pid = make_process(p_context, lister_process_loop, &cfg_dest_lister);

        lister_configuration_t cfg_src_lister;
        cfg_src_lister.mq_key = p_context->shared_key;
        cfg_src_lister.my_recipient_id = MSG_TYPE_TO_SOURCE_ANALYZERS;
        cfg_src_lister.my_receiver_id = MSG_TYPE_TO_SOURCE_LISTER;
        cfg_src_lister.analyzers_count = (the_config->processes_count-2)/2;
        p_context->source_lister_pid = make_process(p_context, lister_process_loop, &cfg_src_lister);

        analyzer_configuration_t cfg_src_analyser;
        cfg_src_analyser.mq_key = p_context->shared_key;
        cfg_src_analyser.my_recipient_id = MSG_TYPE_TO_SOURCE_LISTER;
        cfg_src_analyser.my_receiver_id = MSG_TYPE_TO_SOURCE_ANALYZERS;
        cfg_src_analyser.use_md5 = the_config->uses_md5;   
        p_context->source_analyzers_pids = (pid_t*)malloc(sizeof(pid_t) * (the_config->processes_count - 2) / 2);
        for (int i = 0; i < (the_config->processes_count - 2) / 2; i++) {
            p_context->source_analyzers_pids[i] = make_process(p_context, analyzer_process_loop, &cfg_src_analyser);
        }

        analyzer_configuration_t cfg_dest_analyser;
        cfg_dest_analyser.mq_key = p_context->shared_key;
        cfg_dest_analyser.my_recipient_id = MSG_TYPE_TO_DESTINATION_LISTER;
        cfg_dest_analyser.my_receiver_id = MSG_TYPE_TO_DESTINATION_ANALYZERS;
        cfg_dest_analyser.use_md5 = the_config->uses_md5;   
        p_context->destination_analyzers_pids = (pid_t*)malloc(sizeof(pid_t) * (the_config->processes_count - 2) / 2);
        for (int i = 0; i < (the_config->processes_count - 2) / 2; i++) {
            p_context->destination_analyzers_pids[i] = make_process(p_context, analyzer_process_loop, &cfg_dest_analyser);
        }
    
    }

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
        perror("\nERROR with fork!");
        return -1;
    } else if(pid > 0) {
        p_context->processes_count++;
        return pid;
    } else {
        return -1 ;
    }
}

/*!
 * @brief lister_process_loop is the lister process function (@see make_process)
 * @param parameters is a pointer to its parameters, to be cast to a lister_configuration_t
 */
void lister_process_loop(void *parameters) {
    
    return 0; 
    
}

/*!
 * @brief analyzer_process_loop is the analyzer process function
 * @param parameters is a pointer to its parameters, to be cast to an analyzer_configuration_t
 */
void analyzer_process_loop(void *parameters) {
    return 0;
}

/*!
 * @brief clean_processes cleans the processes by sending them a terminate command and waiting to the confirmation
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the processes context
 */
void clean_processes(configuration_t *the_config, process_context_t *p_context) {
    // Do nothing if not parallel

    if (the_config == NULL || p_context == NULL) {
        perror("ERROR dans parametres de clean_processes\n");
        return;
    }

    if (!the_config->is_parallel) {
        return;
    }

    any_message_t msg;

    // Send terminate
   
    send_terminate_command(p_context->message_queue_id, MSG_TYPE_TO_SOURCE_LISTER);
    send_terminate_command(p_context->message_queue_id, MSG_TYPE_TO_DESTINATION_LISTER);

    for (int i = 0; i < (p_context->processes_count - 2) / 2; i++) {
        send_terminate_command(p_context->message_queue_id, MSG_TYPE_TO_SOURCE_ANALYZERS);
        send_terminate_command(p_context->message_queue_id, MSG_TYPE_TO_DESTINATION_ANALYZERS);
    }

    // Wait for responses
    
    for (int i = 0; i < p_context->processes_count; i++) {
        msgrcv(p_context->message_queue_id, &msg, sizeof(any_message_t) - sizeof(long), MSG_TYPE_TO_MAIN, 0);
    }

    // Free allocated memory
     free(p_context->source_analyzers_pids);
     free(p_context->destination_analyzers_pids);

     if (msgctl(p_context->message_queue_id, IPC_RMID, NULL) == -1) {
        perror("ERROR freeing message queue\n");
    }
}

