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
    lister_configuration_t *config = (lister_configuration_t *)parameters;

    // Initialisation de la liste des fichiers
    files_list_t files_list;
    init_files_list(&files_list);

    // Parcourir le répertoire et construire la liste des fichiers
    make_list(&files_list, config->directory_to_list);  // Remplacer par le chemin approprié

    // Envoi des demandes d'analyse
    int current_analyzers = 0;
    files_list_entry_t *current_entry = files_list.head;
    while (current_entry != NULL && current_analyzers < config->analyzers_count) {
        // Envoi de la demande d'analyse du fichier courant
        request_element_details(config->message_queue_id, current_entry, config, &current_analyzers);

        // Passage au fichier suivant
        current_entry = current_entry->next;
    }
 clear_files_list(&files_list);
}

/*!
 * @brief analyzer_process_loop is the analyzer process function
 * @param parameters is a pointer to its parameters, to be cast to an analyzer_configuration_t
 */
void analyzer_process_loop(void *parameters) {
    analyzer_configuration_t *config = (analyzer_configuration_t *)parameters;
    bool running = true;
    any_message_t message;

    while (running) {
        // Attendre un message de demande d'analyse
        if (msgrcv(config->message_queue_id, &message, sizeof(analyze_file_command_t) - sizeof(long), config->my_receiver_id, 0) == -1) {
            perror("Erreur lors de la réception du message d'analyse");
            continue;
        }

        if (message.analyze_file_command.op_code == COMMAND_CODE_ANALYZE_FILE) {
            // Traitement de l'analyse de fichier
            files_list_entry_t *entry = &message.analyze_file_command.payload;

            // Effectuer l'analyse ici (taille, date de modification, éventuellement MD5)
            get_file_stats(entry, config->use_md5); // Cette fonction doit être implémentée pour effectuer l'analyse

            // Préparer et envoyer la réponse
            send_analyze_file_response(config->message_queue_id, config->my_recipient_id, entry);
        } else if (message.analyze_file_command.op_code == COMMAND_CODE_TERMINATE) {
            running = false;
        }
    }
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

