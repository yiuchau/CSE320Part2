#define _GNU_SOURCE
#include "csapp.h"
#include <unistd.h>

int main(int argc, char** argv) {
	int parentfd = atoi(argv[1]);
	fprintf(stderr, "Parent's fd: %d\n", parentfd);
	char buf[MAXLINE];
	memset(buf, 0, MAXLINE);

	fd_set read_set, ready_set;


    if (Read(parentfd, buf, MAXLINE) <= 0) {
    	Close(parentfd);
        exit(0);
    }
    
    Write(STDOUT_FILENO, buf, sizeof(buf));
    printf("\n");


    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(parentfd, &read_set);     /* Add listenfd to read set */ 





    while (1) {
        ready_set = read_set;
        Select(parentfd + 1, &ready_set, NULL, NULL, NULL); 

        if (FD_ISSET(parentfd, &ready_set)) { 
            /* Read from clientfd */
            //fprintf(stderr, "\x1B[1;34mReading chat from parent...\n");
            //fflush(stderr);
            memset(buf, 0, MAXLINE);
    		if (Read(parentfd, buf, MAXLINE) <= 0) {
    			Close(parentfd);
        		exit(0);
    		}

    		//printf("<");
    		Write(STDOUT_FILENO, buf, sizeof(buf));
    		printf("\n");
        }else
        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
        	memset(buf, 0, MAXLINE);
    		if (!Fgets(buf, MAXLINE, stdin))
        		exit(0); /* EOF */
        	if(strncmp(buf, "/close", 6) == 0) {
        		printf("Closing.. \n");
        		Close(parentfd);
        		exit(0);
        	}else {
        		//printf("\x1B[1;34mSending input to parent.. \n");

        		
    			//printf(">");
        		//Write(STDOUT_FILENO, buf, strlen(buf) - 1);
        		//printf("\n");
        		Write(parentfd, buf, strlen(buf) - 1);
        	}

        }
    }

}