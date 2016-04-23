#include "csapp.h"
void command(int clientfd);

int main(int argc, char **argv) 
{
    int clientfd;
    char *username, *host;
    int port;
    fd_set read_set, ready_set;

    if (argc < 4) {
    //fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    //print usage
    exit(0);
    }

    username = argv[1];
    host = argv[2];
    port = atoi(argv[3]);


    clientfd = Open_clientfd(host, port);
    // write username to server to login
    Write(clientfd, username, strlen(username));
    // check error on login, print usage 

    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(clientfd, &read_set);     /* Add listenfd to read set */ 

    while (1) {
        ready_set = read_set;
        Select(clientfd+1, &ready_set, NULL, NULL, NULL); 
        if (FD_ISSET(STDIN_FILENO, &ready_set)) 
            fprintf(stdout, "\x1B[1;34mReading command line from stdin.\n");
            fflush(stdout);
            command(clientfd); /* Read command line from stdin */
        if (FD_ISSET(clientfd, &ready_set)) { 
            /* Read from clientfd */
        }
    }

    Close(clientfd);
    exit(0);
}

void command(int clientfd) {
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0); /* EOF */
    printf("%s", buf); /* Process the input command */

    if (strcmp(buf, "/time") == 0) {
        // asks server for seconds elapsed
    } 
    else if (strcmp(buf, "/help") == 0) {
        // list all client commands
    }
    else if (strcmp(buf, "/listu") == 0) {
        // dump lists of currently logged users
    }
    else if (strcmp(buf, "/logout") == 0) {
        // disconnect with server

        Close(clientfd);
        exit(0);
    }
    return;
}
/* $end select */


