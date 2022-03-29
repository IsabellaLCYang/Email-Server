#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

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
    } else {
        dlog("Welcome message sent unsuccessfully \n");
    }

    // read line from socket
    char *parts[MAX_LINE_LENGTH+1];
    int read = nb_read_line(nb, recvbuf);
    if (read != -1){
        split(recvbuf, parts);
        int size = sizeof parts / sizeof parts[0];
        if (size > 2){
            send_formatted(fd, "-ERR invalid arguments \r\n");
        } else if (strcmp(parts[0], "USER") == 0){
            // check if name in users.txt
            if (is_valid_user(parts[1], NULL) != 0){
                // username exists
                send_formatted(fd, "+OK %s is a valid mailbox \r\n", parts[1]);
            } else {
                // username not valid
                send_formatted(fd, "-ERR no mailbox for %s \r\n", parts[1]);
            }
        } else {
            // client did not send USER message
            send_formatted(fd, "-ERR \r\n");
        }
    }
  
    nb_destroy(nb);

    return;
}
