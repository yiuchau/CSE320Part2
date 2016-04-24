#include "csapp.h"

void command();

char buf[MAXLINE];
char *protocol_end = " \r\n\r\n";
int plen = 5;
size_t buf_used;

int clientfd;

int vflag = 0, cflag = 0;

void read_socket(int fd) {
}


#define Usage printf("./client [-hcv] NAME SERVER_IP SERVER_PORT\n \
-h         Displays help menu & returns EXIT_SUCCESS\n \
-c         Requests server to create a new use\n \
-v         Vebose pring all incoming and outcoming protcol verbs & content. \
NAME       This is the username to display when chatting. \
SERVER_IP  The ipaddress of server to connect to.\
SERVER_PORT Port number to connect to.\n");

void sigintHandler(int sig){

    fprintf(stderr, "\x1B[1;31m :SIGINT CAUGHT Close(%d)\n", clientfd);
    Close(clientfd);
    exit(0);
}

int main(int argc, char **argv) 
{
    //int clientfd;
    char *username, *host;
    int port;
    char opt;
    fd_set read_set, ready_set;
    //plen = strlen(protocol_end);


    if (argc < 4) {
        Usage;
        exit(0);
    }

    while((opt = getopt(argc, argv, "hcv")) != -1) {
        switch(opt) {
            case 'h':
                fprintf(stderr, "The help menu was selected\n");
                Usage;
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                fprintf(stderr, "Verbosity flag provided\n");
                vflag++;
                break;
            case 'c':
                fprintf(stderr, "User account creation requested.\n");
                cflag++;
                break;
            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.
                 */
            default:
                fprintf(stderr,"A bad option was provided.\n");
                Usage;
                exit(EXIT_FAILURE);
                break;
        }
    }


    username = argv[argc-3];
    host = argv[argc-2];
    port = atoi(argv[argc-1]);


    clientfd = Open_clientfd(host, port);
    // write username to server to login following wolfie protcol
    //strcpy(buf, "WOLFIE \r\n\r\n");
    Write(clientfd, "WOLFIE", 6);
    Write(clientfd, protocol_end, plen);

    memset(buf, 0, sizeof(buf));

    Read(clientfd, buf, sizeof(buf));

    //fprintf(stderr, "buf:%s\n", buf);

    if(strncmp("EIFLOW \r\n\r\n", buf, 11) == 0) {

        memset(buf, 0, sizeof(buf));
        
        strncpy(buf, "I AM ", 5);
        strncpy(&buf[5], username, strlen(username));
        strncpy(&buf[5 + strlen(username)], protocol_end, plen);
        Write(clientfd, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));

        Read(clientfd, buf, sizeof(buf));


        if(strncmp("HI ", buf, 3) == 0 &&
            strncmp(username, &buf[3], strlen(username)) == 0 &&
            strncmp(protocol_end, &buf[3 + strlen(username)], plen) == 0) {
                //successfully connected!
            memset(buf, 0, sizeof(buf));
            Read(clientfd, buf, sizeof(buf));
            Write(STDOUT_FILENO, &buf[5], (size_t)(strstr(buf, protocol_end) - &buf[4]));

        }else if (strncmp("ERR 00 USER NAME TAKEN", buf, 22) == 0 &&
            strncmp(protocol_end, &buf[22], plen) == 0) {
                //user taken
            if(strncmp("BYE", &buf[22 + plen], 3) == 0 &&
                strncmp(protocol_end, &buf[22 + plen + 3], plen) == 0){
                //write byte
                memset(buf, 0, sizeof(buf));
                strncpy(buf, "BYE", 3);
                strncpy(&buf[3], protocol_end, plen);
                Write(clientfd, buf, strlen(buf));
                fprintf(stderr, "Username taken, closing connection. \n");
                Close(clientfd);
                //exit
                exit(EXIT_FAILURE);
            }else{
                fprintf(stderr, "Expected: BYE, invalid response from server: %s\n", buf);
            }
        }else{

            fprintf(stderr, "Expected: HI <Username> || ERR 00 USER NAME TAKEN, invalid response from server: %s\n", buf);
        }
    }else{
        fprintf(stderr, "Expected: EIFLOW, invalid response from server: %s\n", buf);
    }

    //Write(clientfd, username, strlen(username));
    // check error on login, print usage
     

    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(clientfd, &read_set);     /* Add listenfd to read set */ 

    while (1) {
        ready_set = read_set;
        Select(clientfd+1, &ready_set, NULL, NULL, NULL); 
        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
            //fprintf(stderr, "\x1B[1;34mReading command line from stdin.\n");
            //fflush(stderr);
            command(); /* Read command line from stdin */
        }
        if (FD_ISSET(clientfd, &ready_set)) { 
            /* Read from clientfd */
            //fprintf(stderr, "\x1B[1;34mReading command line from clientfd.\n");
            //fflush(stderr);
        }
    }

    Close(clientfd);
    exit(0);
}

void command() {
    char cmd[MAXLINE];
    if (!Fgets(cmd, MAXLINE, stdin))
        exit(0); /* EOF */

    printf("%s", cmd); /* Process the input command */

    if (strncmp(cmd, "/time", 5) == 0) {

        // asks server for seconds elapsed
        time_t time_elapsed;
        int hours, minutes, seconds;

        fprintf(stderr, "\x1B[1;34mClient command: /time entered\n");

        memset(buf, 0, sizeof(buf));
        strncpy(buf, "TIME", 4);
        strncpy(&buf[4], protocol_end, plen);
        Write(clientfd, buf, strlen(buf));

        Read(clientfd, buf, sizeof(buf));

        if(strncmp("EMIT", buf, 4) == 0 &&
            strstr(buf, protocol_end) != 0){

            time_elapsed = atoi(&buf[5]);
            hours = (int)time_elapsed / 3600;
            time_elapsed = time_elapsed % 3600;
            minutes = (int)time_elapsed / 60;
            seconds = time_elapsed % 60;
            fprintf(stdout, "connected for %d hours %d minutes %d seconds\n", hours, minutes, seconds);

        }else{
            fprintf(stderr, "Expected: BYE, invalid response from server: %s\n", buf);
        }

    } 
    else if (strncmp(cmd, "/help", 5) == 0) {
        // list all client commands

        fprintf(stderr, "\x1B[1;34mClient command: /help entered\n");
        fprintf(stdout, "/help: Print all client commands\n\
                         /time: Get time elapsed for connection\n\
                         /listu: Print all currently logged users\n\
                         /logout: Disconnect with server.\n");
    }
    else if (strncmp(cmd, "/listu", 6) == 0) {
        // dump lists of currently logged users

        fprintf(stderr, "\x1B[1;34mClient command: /listu entered\n");
    }
    else if (strncmp(cmd, "/logout", 8) == 0) {
        // disconnect with server

        fprintf(stderr, "\x1B[1;34mClient command: /logout entered\n");

        Close(clientfd);
        exit(0);
    }
    return;
}
/* $end select */


