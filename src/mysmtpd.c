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

    // send welcome message
    char* hostName = my_uname.nodename;
    if (send_formatted(fd,"220 %s Simple Mail Transfer Service Ready \r\n", hostName) != -1) {
        dlog("Welcome Message sent sucessfully \n");
    } else {
    dlog("Welcome Message sent unsuccesfully \n");
    }

    // process HELO/EHLO
    char *parts[MAX_LINE_LENGTH+1];
    if (recv(fd,recvbuf,MAX_LINE_LENGTH+1,0) != -1) {
        split(recvbuf,parts);
    // check if the client sent HELO or EHLO
        if (strcmp (parts[0], "HELO") == 0) {
            send_formatted(fd,"250 %s \r\n", hostName);
        } else if (strcmp (parts[0], "EHLO") == 0) {
            send_formatted(fd,"250 %s greets %s \r\n", hostName, parts[1]);
        }
    }

    return;

    // First step of mail transactions
    // MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
    // reset buffer
    // memset(recvbuf,"", MAX_LINE_LENGTH+1);

    // // VRFY
    // char *parts[MAX_LINE_LENGTH+1];
    // if (recv(fd,recvbuf,MAX_LINE_LENGTH+1,0) != -1) {
    //     switch (expression)
    //     {
    //     case /* constant-expression */:
    //         /* code */
    //         break;
        
    //     default:
    //         break;
    //     }
    // }

    //  MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
    // take the first <> out , find it 
    // if okay, return 250 
    // if not, determine if the failure is permanent / temporary  , return with 550/553
    nb_destroy(nb);
}

