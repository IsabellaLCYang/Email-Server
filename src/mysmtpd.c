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
#define MAIL_RECEIVED_STATE 2
#define RCPT_RECEIVED_STATE 3
#define INVALID_STATE 100

#define FROM_REGEX "FROM:<.*>"
#define TO_REGEX "TO:<.*>"
#define EMAIL_REGEX ".+@.+"

regex_t from_regex;
regex_t to_regex;
regex_t email_regex;
int from_creation_value;
int to_creation_value;
int email_creation_value;
regmatch_t from_pmatch[1];
regmatch_t to_pmatch[1];
regmatch_t email_pmatch[1];

static void handle_client(int fd);

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

char *isolate_email(char *content)
{
    char *openning_bracket_pointer = strstr(content, "<");
    int lengthIncludingBrackets = strlen(openning_bracket_pointer);
    // check if the username is empty
    if (lengthIncludingBrackets > 2)
    {
        // delete the ending bracket
        openning_bracket_pointer[lengthIncludingBrackets - 1] = '\0';
        // move the pointer to the start of the string instead of the opening bracket
        openning_bracket_pointer++;
        return openning_bracket_pointer;
        // pointer points to array containing only contents with brackets removed
    }
    else
    {
        return NULL;
    }
}

int vrfy_handler(int fd, char *content)
{
    if ((content != NULL) && (strstr(content, "@") != NULL))
    {
        if (is_valid_user(content, NULL) != 0)
        {
            // username exists
            send_formatted(fd, "250 OK \r\n");
            return 1; // valid username
        }
        else
        {
            send_formatted(fd, "550 no such user - %s \r\n", content);
        }
    }
    else
    {
        send_formatted(fd, "501 invalid user - %s \r\n", content);
    }
    return 0; // invalid username
}

int welcome_state_handler(char *hostName, char *command, char *content, int fd)
{
    // parts[0] command
    // parts[1] content
    if (strcmp(command, "HELO") == 0)
    {
        send_formatted(fd, "250 %s \r\n", hostName);
        return HELO_EHLO_RECEIVED_STATE;
    }
    else if (strcmp(command, "EHLO") == 0)
    {
        send_formatted(fd, "250 %s greets %s \r\n", hostName, content);
        return HELO_EHLO_RECEIVED_STATE;
    }
    else
    {
        send_formatted(fd, "503 Bad Sqeuence \r\n");
        return INVALID_STATE;
    }
}

void regex_init()
{
    from_creation_value = regcomp(&from_regex, FROM_REGEX, REG_EXTENDED);
    email_creation_value = regcomp(&email_regex, EMAIL_REGEX, REG_EXTENDED);
    to_creation_value = regcomp(&to_regex, TO_REGEX, REG_EXTENDED);
    if ((from_creation_value == 0) || (email_creation_value == 0) || (to_creation_value == 0))
    {
        dlog("Regex compiled \n");
    }
    else
    {
        dlog("Regex compilation failed \n");
    }
}

int helo_ehlo_state_handler(char *command, char *content, int fd)
{
    if (strcmp(command, "MAIL") == 0)
    {
        if (content == NULL)
        {
            send_formatted(fd, "500 Invalid \r\n");
        }
        else
        {
            int from_match_value = regexec(&from_regex, content, 1, from_pmatch, 0);
            if (from_match_value != 0) // not match
            {
                send_formatted(fd, "500 Missing FROM:<> \r\n");
            }
            else
            {
                char *email_address = isolate_email(content);
                if (email_address != NULL)
                {
                    int email_match_value = regexec(&email_regex, email_address, 1, email_pmatch, 0);
                    if (email_match_value == 0)
                    { // match
                        send_formatted(fd, "250 OK \r\n");
                        return MAIL_RECEIVED_STATE;
                    }
                }
                // empty adress or invalid addess
                send_formatted(fd, "501 Invalid email address \r\n");
            }
        }
    }
    else
    {
        send_formatted(fd, "503 Bad Sequence \r\n");
    }
    return HELO_EHLO_RECEIVED_STATE;
}

// src_of_invoktion : 0 if invoked from handle_client, 1 if invoked from rcpt_receoved_handler
int mail_received_handler(char *command, char *content, int fd,int src_of_invoktion)
{
    if (strcmp(command, "RCPT") == 0)
    {
        int to_match_value = regexec(&to_regex, content, 1, to_pmatch, 0);
        if (to_match_value == 0) // match
        {
            char *email_address = isolate_email(content);
            if (email_address != NULL)
            {
                int email_match_value = regexec(&email_regex, email_address, 1, email_pmatch, 0);
                if (email_match_value == 0) // match regex
                {
                    if (is_valid_user(email_address, NULL) != 0)
                    {
                        // add user to list
                        user_list_t list = create_user_list();
                        add_user_to_list(&list,email_address);
                        send_formatted(fd, "250 OK \r\n");
                        return RCPT_RECEIVED_STATE;
                    } else {
                        send_formatted(fd, "550 No such user \r\n");
                    }
                } else {
                    send_formatted(fd, "501 Invalid email address \r\n");
                }
            }
        }
        else
        {
            send_formatted(fd, "500 Invalid \r\n");
        }
    }
    else
    {
        send_formatted(fd, "503 Bad Sequence \r\n");
    }
    if (src_of_invoktion == 0) {
    return MAIL_RECEIVED_STATE;
    } else {
        return RCPT_RECEIVED_STATE;
    }
}

int rcpt_received_handler(char *command, char *content, int fd) {
     if (strcmp(command, "RCPT") == 0) {
         mail_received_handler(command,content,fd,1);
     } else if (strcmp(command, "DATA") == 0) {

     }
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
    }

    regex_init();

    // read line from socket
    int read = nb_read_line(nb, recvbuf);
    while ((read != -1) && (read != 0))
    {
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
            vrfy_handler(fd, parts[1]);
        }
        // sequential commands
        else
        {
            switch (state)
            {
            // welcome message sent
            case 0:
                // check if the client sent HELO or EHLO
                state = welcome_state_handler(hostName, parts[0], parts[1], fd);
                break;
            case 1:;
                state = helo_ehlo_state_handler(parts[0], parts[1], fd);
                break;
            case 2:;
                state = mail_received_handler(parts[0], parts[1], fd,0);
                break;
            case 3:;
            break;
            }
        }
        read = nb_read_line(nb, recvbuf);
    }
    nb_destroy(nb);
    close(fd);
    exit(0);
    return;
}
