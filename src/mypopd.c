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
    
    if (send_formatted(fd, "+OK POP3 server ready") != -1){
        dlog("Welcome message sent sucessfully \n");
    } else {
        dlog("Welcome message sent unsuccessfully \n");
    }
  
    nb_destroy(nb);

    return;
}
