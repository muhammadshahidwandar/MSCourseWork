#include <sys/socket.h> 

#include <netdb.h> 
 
#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 10000


void chatUser(char *hostname, int aPort);
 
main (int argc, char *argv[]) 
{ 
    int i,aPort =7777;
    char hostname[64];  

    if(argc != 2)

    {

        fprintf(stderr, "Client-Usage: %s the_client_hostname\n", argv[0]);

        // just exit

        exit(1);

    }
    // printf("usage:%s [-p port] [-h hostname] \n" , argv[0]);
    gethostname(hostname, sizeof(hostname));

    printf("\nStarting chat session: host(%s) port(%d)\n",hostname, aPort);    
    
    chatUser(hostname, aPort);
    
    exit(0); 
} 

 

void chatUser(char *hostname, int aPort)
{  
    unsigned char buffer[BUFF_SIZE]; 
    struct sockaddr_in sin; 
    struct hostent *hp; 
    struct timeval stv; 
    int i, nread; 
    int fd_chat  = -1; 
    int iMaxFd, iNumInputs, nContinue = 1; 
    fd_set fdCheck; 
    
    // Now we need to create a socket connection with the chat server
    // 
 
// Get the IP address of the hostname

    hp = gethostbyname(hostname); 
    if (hp == NULL) 
    { 
        printf("\n%s: unknown host.\n", hostname); 
        nContinue = 0; 
    } 
    else 
    { 

// Create a socket and connect to the chat server

        if ((fd_chat  = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        { 
            perror("client: socket()"); 
            nContinue = 0; 
        } 
        else 
        {    
            sin.sin_family = AF_INET; 
            sin.sin_port = htons(aPort); 
            memcpy(&sin.sin_addr, hp->h_addr, hp->h_length); 
            
            if (connect(fd_chat , (struct sockaddr *)&sin, sizeof(sin)) < 0) 
            { 
                perror("client: connect()"); 
                nContinue = 0; 
            } 
        } 
    } 
    
    // Now that we have connected to the other end, we will sit waiting for
    // input from either the fd_chat socket or from STDIN.  
    // When we read from fd_chat, we write the received buffer to STDOUT.
    // When we receive from the keyboard (STDIN), we send the contents to
    // the chat server.
    // The select statement is the key call in this routine. 
    
    for(;nContinue;) 
    { 
        stv.tv_sec = 0; 
        stv.tv_usec = 0; 
        FD_ZERO(&fdCheck); 
        iMaxFd = STDIN_FILENO; 
        if (fd_chat  > STDIN_FILENO  ) iMaxFd = fd_chat ;
        
        iMaxFd += 1; 

  // Set up mask to look for input from fd_chat or STDIN        
        FD_SET(STDIN_FILENO  , &fdCheck); 
        FD_SET(fd_chat , &fdCheck); 

        iNumInputs = select(iMaxFd, &fdCheck, 0, 0, NULL /* &stv*/); 
        if (iNumInputs == -1) 
        { 
            /* Error*/ 
            printf("\n Bad return from select ==> die\n"); 
            nContinue = 0; 
        } 
        else if (iNumInputs == 0) 
        { 
            /* Timeout */ 
            printf("\n Timeout from select\n"); 
            nContinue = 0;
        } 
        else if (FD_ISSET(fd_chat  , &fdCheck)) 
        { 
            // Can safely read without blocking
            nread = read(fd_chat, buffer, BUFF_SIZE);
            if (nread > 0)
                {
                // Received from chat server
                    buffer[nread] = 0;                   
                    write(STDOUT_FILENO, buffer, nread);
                }
            else
            {
                // chat server socket seems to have disconnected
                 
                if (nread == 0) nContinue = 0;
                if (nread < 0)
                    fprintf(stderr,"fd_chat err:%s\n", 
                        strerror(errno));
            }

        } 
        else if (FD_ISSET(STDIN_FILENO , &fdCheck)) 
        { 
            // can safely read without blocking
            nread = read(STDIN_FILENO, buffer, BUFF_SIZE);
            if (nread > 0)
            {
            // Keyboard input that needs to be sent to the chat server

                buffer[nread] = 0;
                write(fd_chat, buffer, nread);
            }
        } 
    } 
    if (fd_chat  >= 0) 
        close(fd_chat ); 
    exit(0); 
}
