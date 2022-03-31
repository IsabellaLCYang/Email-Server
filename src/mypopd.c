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

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    int state;
    char username[MAX_LINE_LENGTH];
  
    /* TO BE COMPLETED BY THE STUDENT */

    // TCP connection established: send greeting message, enter AUTHORIZATION state
    // client identifies self, enter TRANSACTION state
    // clent requests actions
    // client issues QUIT command, enter UPDATE state, release resources, say goodbye

    // server responds to unrecognized, unimplemented, syntactically invalid command, or command in wrong state, 
    //      with negative status indicator ("-ERR")

    // send greeting message
    if (send_formatted(fd, "+OK POP3 server ready \r\n") != -1){
        dlog("Welcome message sent sucessfully \n");
        state = WELCOME_SENT_STATE;
    } else {
        dlog("Welcome message sent unsuccessfully \n");
    }

    // read line from socket
    int read = nb_read_line(nb, recvbuf);
    while ((read != -1) && (read != 0) && (state != QUIT_SENT_STATE)){
        char *parts[MAX_LINE_LENGTH+1];
        split(recvbuf, parts);
        switch(state){
            case WELCOME_SENT_STATE:
                if (strcmp(parts[0], "USER") == 0){
                    if ((parts[2] != NULL) || (parts[1] == NULL)){
                        // wrong number of arguments
                        send_formatted(fd, "-ERR invalid arguments \r\n");
                    } else if (is_valid_user(parts[1], NULL) != 0){
                        // username exists
                        send_formatted(fd, "+OK %s is a valid mailbox \r\n", parts[1]);
                        state = USER_SENT_STATE;
                        strcpy(username, parts[1]);
                    } else {
                        // username not valid
                        send_formatted(fd, "-ERR no mailbox for %s \r\n", parts[1]);
                    }
                } else if (strcmp(parts[0], "QUIT") == 0){
                    if (parts[1] != NULL){
                        // should have no arguments
                        send_formatted(fd, "-ERR invalid arguments \r\n");
                    } else {
                        send_formatted(fd, "+OK POP3 server signing off \r\n");
                        state = QUIT_SENT_STATE;
                    }
                } else {
                    // USER or QUIT not sent, wrong command order
                    send_formatted(fd, "-ERR need USER name \r\n");
                }
                break;
            case USER_SENT_STATE:
                if (strcmp(parts[0], "PASS") == 0){
                    dlog(username);
                    if (parts[1] == NULL){
                        // no string password arg provided
                        send_formatted(fd, "-ERR invalid arguments \r\n");
                    } else if (is_valid_user(username, parts[1]) != 0){
                        // password valid for username
                        send_formatted(fd, "+OK maildrop ready \r\n");
                        state = PASS_SENT_STATE;
                    } else {
                        // password invalid
                        send_formatted(fd, "-ERR invalid password \r\n");
                    }
                } else if (strcmp(parts[0], "QUIT") == 0){
                    if (parts[1] != NULL){
                        // should have no arguments
                        send_formatted(fd, "-ERR invalid arguments \r\n");
                    } else {
                        send_formatted(fd, "+OK POP3 server signing off \r\n");
                        state = QUIT_SENT_STATE;
                    }
                } else {
                    // PASS or QUIT not sent, wrong command order
                    send_formatted(fd, "-ERR need PASS \r\n");
                }
                break;
        }
        if (state == QUIT_SENT_STATE){
            break;
        }
        read = nb_read_line(nb,recvbuf);
    }
    close(fd);
    nb_destroy(nb);
    exit(0);
    return;
}
