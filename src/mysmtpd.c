#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <regex.h>

#define MAX_LINE_LENGTH 1024
#define WELCOME_SENT_STATE 0
#define HELO_EHLO_RECEIVED_STATE 1
#define INVALID_STATE 100

static void handle_client(int fd);
// test
// test 2

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }

    run_server(argv[1], handle_client);

    return 0;
}

void handle_client(int fd)
{
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    int state;

    struct utsname my_uname;
    uname(&my_uname);

    // send welcome message
    char *hostName = my_uname.nodename;
    if (send_formatted(fd, "220 %s Simple Mail Transfer Service Ready \r\n", hostName) != -1)
    {
        state = WELCOME_SENT_STATE;
        dlog("Welcome Message sent sucessfully \n");
    }
    else
    {
        dlog("Welcome Message sent unsuccesfully \n");
    }

    // read line from socket
    int read = nb_read_line(nb, recvbuf);
    while ((read != -1) && (read != 0))
    {
        // process HELO/EHLO
        char *parts[MAX_LINE_LENGTH + 1];
        split(recvbuf, parts);
        // generic commands
        if (strcmp(parts[0], "NOOP") == 0)
        {
            send_formatted(fd, "250 OK \r\n");
        }
        else if (strcmp(parts[0], "QUIT") == 0)
        {
            send_formatted(fd, "221 OK \r\n");
            read = nb_read_line(nb, recvbuf);
            break;
            // close connection
        }
        else if (strcmp(parts[0], "VRFY") == 0)
        {
            if (strchr(parts[1], '@') != NULL)
            {
                if (is_valid_user(parts[1], NULL) != 0)
                {
                    // username exists
                    send_formatted(fd, "250 %s \r\n", parts[1]);
                }
                else
                {
                    send_formatted(fd, "550 no such user - %s\r\n", parts[1]);
                }
            } else {
                send_formatted(fd, "550 no such user - %s\r\n", parts[1]);
            }
        }
        else
        {
            // sequential commands
            switch (state)
            {
            // welcome message sent
            case 0:
                // check if the client sent HELO or EHLO
                welcome_state_handler(hostName, parts[0], parts[1], fd, state);
            }
        }
        read = nb_read_line(nb, recvbuf);
    }

    // First step of mail transactions
    // MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
    // reset buffer
    // memset(recvbuf,"", MAX_LINE_LENGTH+1);


    //  MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
    // take the first <> out , find it
    // if okay, return 250
    // if not, determine if the failure is permanent / temporary  , return with 550/553
    nb_destroy(nb);
    return;
}

void welcome_state_handler(char *hostName, char *command, char *content, int fd, int *state)
{
    // parts[0] command
    // parts[1] content
    if (strcmp(command, "HELO") == 0)
    {
        send_formatted(fd, "250 %s \r\n", hostName);
        state = HELO_EHLO_RECEIVED_STATE;
    }
    else if (strcmp(command, "EHLO") == 0)
    {
        send_formatted(fd, "250 %s greets %s \r\n", hostName, content);
        state = HELO_EHLO_RECEIVED_STATE;
    }
    else
    {
        send_formatted(fd, "503 Bad Sqeuence \r\n");
        state = INVALID_STATE;
    }
}