#include <messages.h>
#include <sys/msg.h>
#include <string.h>

// Functions in this file are required for inter processes communication

/*!
 * @brief send_file_entry sends a file entry, with a given command code
 * @param msg_queue the MQ identifier through which to send the entry
 * @param recipient is the id of the recipient (as specified by mtype)
 * @param file_entry is a pointer to the entry to send (must be copied)
 * @param cmd_code is the cmd code to process the entry.
 * @return the result of the msgsnd function
 * Used by the specialized functions send_analyze*
 */
int send_file_entry(int msg_queue, int recipient, files_list_entry_t *file_entry, int cmd_code) {
    any_message_t message;
    message.list_entry.mtype = (long)recipient;
    message.list_entry.op_code = (char)cmd_code;                  // can i use pointer notation here

    memcpy(&(message.list_entry.payload), file_entry, sizeof(files_list_entry_t));

    int result = msgsnd(msg_queue, &message, sizeof(files_list_entry_transmit_t) - sizeof(long), 0);

    if (result == -1) {
        perror("\nmsgsnd in send_file_entry");
    }

    return result;
}

/*!
 * @brief send_analyze_dir_command sends a command to analyze a directory
 * @param msg_queue is the id of the MQ used to send the command
 * @param recipient is the recipient of the message (mtype)
 * @param target_dir is a string containing the path to the directory to analyze
 * @return the result of msgsnd
 */
int send_analyze_dir_command(int msg_queue_id, int dest, const char *dir) {
    analyze_dir_command_t cmd;
    cmd.mtype = dest;
    strncpy(cmd.target, dir, sizeof(cmd.target) - 1);
    cmd.target[sizeof(cmd.target) - 1] = '\0'; // Assurez-vous que la chaîne est terminée correctement.
    cmd.op_code = COMMAND_CODE_ANALYZE_DIR;
    return msgsnd(msg_queue_id, &cmd, sizeof(cmd) - sizeof(long), 0);
}

// The 3 following functions are one-liners

/*!
 * @brief send_analyze_file_command sends a file entry to be analyzed
 * @param msg_queue the MQ identifier through which to send the entry
 * @param recipient is the id of the recipient (as specified by mtype)
 * @param file_entry is a pointer to the entry to send (must be copied)
 * @return the result of the send_file_entry function
 * Calls send_file_entry function
 */
int send_analyze_file_command(int msg_queue, int recipient, files_list_entry_t *file_entry) {
    int cmd_code = COMMAND_CODE_ANALYZE_FILE;

    return send_file_entry(msg_queue, recipient, file_entry, cmd_code);
}

/*!
 * @brief send_analyze_file_response sends a file entry after analyze
 * @param msg_queue the MQ identifier through which to send the entry
 * @param recipient is the id of the recipient (as specified by mtype)
 * @param file_entry is a pointer to the entry to send (must be copied)
 * @return the result of the send_file_entry function
 * Calls send_file_entry function
 */
int send_analyze_file_response(int msg_queue, int recipient, files_list_entry_t *file_entry) {
    int cmd_code = COMMAND_CODE_FILE_ANALYZED;

    // Appelle la fonction send_file_entry avec le code de commande approprié
    return send_file_entry(msg_queue, recipient, file_entry, cmd_code);
}
/*!
 * @brief send_files_list_element sends a files list entry from a complete files list
 * @param msg_queue the MQ identifier through which to send the entry
 * @param recipient is the id of the recipient (as specified by mtype)
 * @param file_entry is a pointer to the entry to send (must be copied)
 * @return the result of the send_file_entry function
 * Calls send_file_entry function
 */
int send_files_list_element(int msg_queue, int recipient, files_list_entry_t *file_entry) {
    // le code COMMAND_CODE_FILE_ENTRY est défini dans message.h
    int cmd_code = COMMAND_CODE_FILE_ENTRY;
    // Appelle send_file_entry avec le code de commande approprié
    return send_file_entry(msg_queue, recipient, file_entry, cmd_code);
}

/*!
 * @brief send_list_end sends the end of list message to the main process
 * @param msg_queue is the id of the MQ used to send the message
 * @param recipient is the destination of the message
 * @return the result of msgsnd
 */
int send_list_end(int msg_queue, int recipient) {
    any_message_t message;

    // Configurer le type de message et le code de commande
    message.simple_command.mtype = (long)recipient;
    message.simple_command.message = COMMAND_CODE_LIST_COMPLETE;

    // Envoyer le message
    int result = msgsnd(msg_queue, &message, sizeof(simple_command_t) - sizeof(long), 0);

    if (result == -1) {
        perror("Erreur lors de l'envoi du message avec msgsnd dans send_list_end");
    }

    return result;
}

/*!
 * @brief send_terminate_command sends a terminate command to a child process so it stops
 * @param msg_queue is the MQ id used to send the command
 * @param recipient is the target of the terminate command
 * @return the result of msgsnd
 */
int send_terminate_command(int msg_queue, int recipient) {

    if (recipient <= 0) {
        perror("Identifiant de destinataire invalide\n");
        return -1;
    }

    pid_t recipient_parent = getppid(); // Identifiant du parent du destinataire
    if (recipient_parent != recipient) {
        perror("Erreur envoi à un processus non enfant\n");
        return -1;
    }
    any_message_t message;

    // Configuration  type de message + code de commande pour la terminaison
    message.simple_command.mtype = (long)recipient; // Type de message = destinataire
    message.simple_command.message = COMMAND_CODE_TERMINATE; // Code de commande pour la terminaison (défini dans message.h)
    // Envoi du message à la file de messages
    int result = msgsnd(msg_queue, &message, sizeof(simple_command_t) - sizeof(long), 0);

    // Indique la réussite ou non de l'envoi du message
    if (result != -1) {
        printf("Commande de terminaison envoyée avec succès au PID : %d.\n", recipient);
     } else { 
        perror("Erreur");
    }

    return result;
}

/*!
 * @brief send_terminate_confirm sends a terminate confirmation from a child process to the requesting parent.
 * @param msg_queue is the id of the MQ used to send the message
 * @param recipient is the destination of the message
 * @return the result of msgsnd
 */
int send_terminate_confirm(int msg_queue, int recipient) {

    if (recipient <= 0) {
        perror("Identifiant de destinataire invalide\n");
        return -1;
    }

    // Vérification processus appelant = processus fils
    pid_t calling_process_parent = getppid(); // Identifiant du parent du processus appelant
    if (calling_process_parent == recipient) {
        perror("Tentative d'envoi par un processus parent\n");
        return -1;
    }

    // Vérification destinataire = processus parent
    pid_t parent_pid = getppid(); // Identifiant du parent du processus en cours
    if (parent_pid != recipient) {
        perror("Le destinataire n'est pas un processus parent\n");
        return -1;
    }

    any_message_t message;

    // Configuration type de message + code de commande pour la confirmation de terminaison
    message.simple_command.mtype = (long)recipient; // Type de message = destinataire (généralement le parent)
    message.simple_command.message = COMMAND_CODE_TERMINATE_OK; // Code de commande pour la confirmation de terminaison

     int result = msgsnd(msg_queue, &message, sizeof(simple_command_t) - sizeof(long), 0);

    if (result != -1) {
        printf("Confimation de terminaison envoyée avec succès");
     } else { 
        perror("Erreur");
    }

    return result;
}
