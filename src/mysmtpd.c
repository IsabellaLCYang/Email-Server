#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
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

    struct utsname my_uname;
    uname(&my_uname);
    if (recv(fd,recvbuf,MAX_LINE_LENGTH+1,0) != -1) {
        
    }


    printf("%s\n", recvbuf);
  
    /* TO BE COMPLETED BY THE STUDENT */

    // read from client
    // switch in the message received and 
    // create handler fo each command
    // first messgae has to be HELO/ EHLO
    //  MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
    // take the first <> out , find it 
    // if okay, return 250 
    // if not, determine if the failure is permanent / temporary  , return with 550/553
    nb_destroy(nb);
}

