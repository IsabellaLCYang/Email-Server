#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define WELCOME_SENT_STATE 0
#define USER_SENT_STATE 1
#define PASS_SENT_STATE 2
#define QUIT_SENT_STATE 10

static void handle_client(int fd);

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
  
    run_server(argv[1], handle_client);
  
    return 0;
}

void transaction_handler(int fd, char *command, char *arg, mail_list_t mail_list){
    unsigned int num_emails = get_mail_count(mail_list, 0);
    size_t emails_size = get_mail_list_size(mail_list);
    if (strcmp(command, "NOOP") == 0){
        send_formatted(fd, "+OK\r\n");
    } else if (strcmp(command, "STAT") == 0){
        send_formatted(fd, "+OK %u %zu\r\n", num_emails, emails_size);
    } else if (strcmp(command, "LIST") == 0){
        if (arg == NULL){
            // multiline list all messages
            send_formatted(fd, "+OK %u messages (%zu octets)\r\n", num_emails, emails_size);
            int mail_index = 0;
            while (mail_index < num_emails){
                mail_item_t mail_item = get_mail_item(mail_list, mail_index);
                if (mail_item != NULL){
                    size_t item_size = get_mail_item_size(mail_item);
                    send_formatted(fd, "%d %zu\r\n", (mail_index + 1), item_size);
                }
                mail_index++;
            }
            send_formatted(fd, ".\r\n");
        } else {
            // scan listing for specified message number
            int mail_index = atoi(arg);
            if (mail_index > num_emails){
                send_formatted(fd, "-ERR no such message\r\n");
            } else {
                mail_item_t mail_item = get_mail_item(mail_list, (mail_index - 1));
                if (mail_item == NULL){
                    send_formatted(fd, "-ERR no such message\r\n");
                } else {
                    size_t item_size = get_mail_item_size(mail_item);
                    send_formatted(fd, "+OK %d %zu\r\n", mail_index, item_size);
                }
            }
        }
    } else if (strcmp(command, "RETR") == 0){
        if (arg == NULL){
            // missing arg
            send_formatted(fd, "-ERR need message number\r\n");
        } else {
            int mail_index = atoi(arg);
            if (mail_index > num_emails){
                send_formatted(fd, "-ERR no such message\r\n");
            } else {
                mail_item_t mail_item = get_mail_item(mail_list, (mail_index - 1));
                if (mail_item == NULL){
                    send_formatted(fd, "-ERR no such message\r\n");
                } else {
                    size_t item_size = get_mail_item_size(mail_item);
                    FILE *mail_contents = get_mail_item_contents(mail_item);
                    char c;
                    send_formatted(fd, "+OK %zu octets\r\n", item_size);
                    c = fgetc(mail_contents);
                    while (c != EOF){
                        send_formatted(fd, "%c", c);
                        c = fgetc(mail_contents);
                    }
                    send_formatted(fd, ".\r\n");
                    fclose(mail_contents);
                }
            }
        }
    } else if (strcmp(command, "DELE") == 0){
        if (arg == NULL){
            //missing arg
            send_formatted(fd, "-ERR need message number\r\n");
        } else {
            int mail_index = atoi(arg);
            if (mail_index > num_emails){
                send_formatted(fd, "-ERR no such message\r\n");
            } else {
                mail_item_t mail_item = get_mail_item(mail_list, (mail_index - 1));
                if (mail_item == NULL){
                    send_formatted(fd, "-ERR message %d already deleted\r\n", mail_index);
                } else {
                    mark_mail_item_deleted(mail_item);
                    send_formatted(fd, "+OK message %d deleted\r\n", mail_index);
                }
            }
        }
    }
}

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    unsigned int state;
    char username[MAX_LINE_LENGTH];
    mail_list_t mail_list;

    // send greeting message
    if (send_formatted(fd, "+OK POP3 server ready\r\n") != -1){
        state = WELCOME_SENT_STATE;
    }
    // read line from socket
    int read = nb_read_line(nb, recvbuf);
    while ((read != -1) && (read != 0) && (state != QUIT_SENT_STATE)){
        char *parts[MAX_LINE_LENGTH+1];
        split(recvbuf, parts);
        if (strcmp(parts[0], "QUIT") == 0){
            if (parts[1] != NULL){
                // should have no arguments
                send_formatted(fd, "-ERR invalid arguments\r\n");
            } else {
                send_formatted(fd, "+OK POP3 server signing off\r\n");
                state = QUIT_SENT_STATE;
                destroy_mail_list(mail_list);
                break;
            }
        }
        switch(state){
            case WELCOME_SENT_STATE:
                if (strcmp(parts[0], "USER") == 0){
                    if ((parts[2] != NULL) || (parts[1] == NULL)){
                        // wrong number of arguments
                        send_formatted(fd, "-ERR invalid arguments\r\n");
                    } else if (is_valid_user(parts[1], NULL) != 0){
                        // username exists
                        send_formatted(fd, "+OK %s is a valid mailbox\r\n", parts[1]);
                        state = USER_SENT_STATE;
                        strcpy(username, parts[1]);
                    } else {
                        // username not valid
                        send_formatted(fd, "-ERR no mailbox for %s\r\n", parts[1]);
                    }
                } else {
                    // USER or QUIT not sent, wrong command order
                    send_formatted(fd, "-ERR need USER name\r\n");
                }
                break;
            case USER_SENT_STATE:
                if (strcmp(parts[0], "PASS") == 0){
                    if (parts[1] == NULL){
                        // no string password arg provided
                        send_formatted(fd, "-ERR invalid arguments\r\n");
                    } else if (is_valid_user(username, parts[1]) != 0){
                        // password valid for username
                        send_formatted(fd, "+OK maildrop ready\r\n");
                        state = PASS_SENT_STATE;
                    } else {
                        // password invalid
                        send_formatted(fd, "-ERR invalid password\r\n");
                    }
                } else {
                    // PASS or QUIT not sent, wrong command order
                    send_formatted(fd, "-ERR need PASS\r\n");
                }
                break;
            case PASS_SENT_STATE:
                mail_list = load_user_mail(username);
                transaction_handler(fd, parts[0], parts[1], mail_list);
                break;
        }
        read = nb_read_line(nb,recvbuf);
    }
    close(fd);
    nb_destroy(nb);
    exit(0);
    return;
}
