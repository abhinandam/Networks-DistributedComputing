/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <time.h>
#include <stdarg.h>


#define ECHO_PORT 9999
#define BUF_SIZE 4096

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET
           ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
           : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int write_to_log(FILE *logFile, char *message) {
    time_t rawtime;
    struct tm * timeinfo;

    //va_list ap;
    //va_start(ap, message);

    time(&rawtime);
    timeinfo = localtime(&rawtime );
    fprintf(logFile,"%s\t", asctime(timeinfo));
    fprintf(logFile, "%s", message);
    printf ( "Current local time and date: %s", asctime (timeinfo));

    return 0;
}

int main(int argc, char* argv[])
{
    FILE *logFile;
    logFile = fopen("select_server.log", "w");
    if (logFile == NULL) {
        /* Something is wrong   */
        fprintf(stdout, "Error opening logfile. \n");
        exit(EXIT_FAILURE);
    }

    // set logfile to line buffering
    setvbuf(logFile, NULL, _IOLBF, 0);


    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fd_set master; // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    char remoteIP[INET6_ADDRSTRLEN];

    fprintf(stdout, "----- Echo Server -----\n");
    write_to_log(logFile, "----- Echo Server -----\n");

    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        write_to_log(logFile, "Failed creating socket.\n");

        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        write_to_log(logFile, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        write_to_log(logFile, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }


    FD_ZERO(&master);
    // add the listener to the master set
    FD_SET(sock, &master);

    // keep track of the biggest file descriptor - currently sock
    fdmax = sock;


    /* finally, loop waiting for input and then write it back */
    while (1) {
        read_fds = master; // copy it
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "select");
            write_to_log(logFile, "select");
            return EXIT_FAILURE;
        }

        int i = 0;
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == sock) {
                    // handle new connections
                    cli_size = sizeof(cli_addr);
                    client_sock = accept(sock,
                                         (struct sockaddr *)&cli_addr,
                                         &cli_size);

                    if (client_sock == -1) {
                        close(sock);
                        fprintf(stderr, "Error accepting connection.\n");
                        write_to_log(logFile, "Error accepting connection.\n");
                        return EXIT_FAILURE;
                    } else {
                        FD_SET(client_sock, &master); // add to master set
                        if (client_sock > fdmax) {    // keep track of the max
                            fdmax = client_sock;
                        }
                        printf("selectserver: new connection from %s on "
                                       "socket %d\n",
                               inet_ntop(cli_addr.sin_family,
                                         get_in_addr((struct sockaddr*)&cli_addr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               client_sock);
                        fprintf(logFile,"selectserver: new connection from %s on "
                                       "socket %d\n",
                               inet_ntop(cli_addr.sin_family,
                                         get_in_addr((struct sockaddr*)&cli_addr),
                                         remoteIP, INET6_ADDRSTRLEN),
                                client_sock);
                    }
                }else {
                    readret = 0;
                    // handle data from a client
                    if ((readret = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (readret == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            fprintf(logFile,"selectserver: socket %d hung up\n", i);
                        } else {
                            fprintf(stdout, "Error on receive.\n");
                            write_to_log(logFile, "Error on receive.\n");
                        }

                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set

                    } else {
                        int error = send(i, buf, readret, 0);
                        if (error == -1) {
                            fprintf(stdout, "error in sending");
                            write_to_log(logFile, "error in sending");
                        }
                        fprintf(logFile, "The following string was received by the server: %s\n", buf);
                        memset(buf, '\0', BUF_SIZE);
                        /*
                        int j = 0;
                        // we got some data from a client
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                    int error = send(j, buf, readret, 0);
                                    if (error == -1) {
                                        perror("send");
                                    }
                            }
                        }*/
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
/*
       cli_size = sizeof(cli_addr);
       if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                 &cli_size)) == -1)
       {
           close(sock);
           fprintf(stderr, "Error accepting connection.\n");
           return EXIT_FAILURE;
       }


       readret = 0;

       while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
       {
           buf[0] = 'h';
           buf[1] = 'i';
           buf[2] = '\0';
           if (send(client_sock, buf, readret, 0) != readret)
           {
               close_socket(client_sock);
               close_socket(sock);
               fprintf(stderr, "Error sending to client.\n");
               return EXIT_FAILURE;
           }
           memset(buf, 0, BUF_SIZE);
       }

       if (readret == -1)
       {
           close_socket(client_sock);
           close_socket(sock);
           fprintf(stderr, "Error reading from client socket.\n");
           return EXIT_FAILURE;
       }

       if (close_socket(client_sock))
       {
           close_socket(sock);
           fprintf(stderr, "Error closing client socket.\n");
           return EXIT_FAILURE;
       }
    }

    close_socket(sock);
*/
    write_to_log(logFile, "server being shutdown");
    fclose(logFile);
    closelog();
    return EXIT_SUCCESS;
}

