#include <sys/socket.h> 
#include <netinet/in.h> 

#include <stdio.h>
#include <unistd.h> 
#include <string.h> 
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define NAME_SIZE 40
#define MAX_CHATS 40
#define BUFF_SIZE 10000 
// Message formats
#define chatFull  "Max Chats ... Sorry"
#define welcome_msg "Welcome to chatter:\n Enter your name with a command like:\n-name=Clem\n"
#define change_name "-name="

// Array to contain the sockets for all of the chat users
int fd_chats[MAX_CHATS];
struct name_rec
{
    char name[NAME_SIZE+1];
 };
typedef struct name_rec NAME_REC;
// Array to hold the names for all of the chat users
NAME_REC name_chats[MAX_CHATS];
int numChats = 0;

void server( int aPort);
void chatLoop(int fd_accept);
void processChat(int instance, char * buffer, int nread);
void addChatter(int newConnect);
void remove_chatter(int fd_toRemove);

main (int argc, char **argv) 
{ 
    int i, aPort=7777; 
    char hostname[64];  
    
    fprintf(stderr, "usage:%s [port] \n" , argv[0]); 
     
    if (argc > 1)
        aPort = atoi(argv[1]);
    gethostname(hostname, sizeof(hostname));    
    
    printf("\nI will be your chat server(%s) on port %d\n\n", 
        hostname, aPort);    
  
 // real work happens here
    server(aPort);
    
    exit(0); 
} 


void server(int aPort)
{  
    struct sockaddr_in sin, fsin; 
    int nChildCnt = 0;   
    int fd_connect, fd_accept;   
    int fromlen, nOpt =1; 
    long pid;

    if ((fd_accept = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("server: socket()"); 
        exit(1); 
    } 
 // The following option allows a port to be reused after exitting this
 // server.  Otherwise, we end up waiting for a while 
    setsockopt(fd_accept, SOL_SOCKET, SO_REUSEADDR, &nOpt, sizeof(int)); 
    
    memset(&sin, 0, sizeof(sin)); 
    sin.sin_family = AF_INET; 
    sin.sin_port = htons(aPort); 
    sin.sin_addr.s_addr = htonl(INADDR_ANY); 
    
// Bind socket to port number
    if (bind(fd_accept, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    { 
        perror("server: bind()"); 
        exit(1); 
    } 
    
// Set up Listen queue of 5 simultaneous clients awaiting connect
    if (listen(fd_accept, 5) < 0) 
    { 
        perror("server: listen()"); 
        exit(1); 
    } 
// Notice we don't block on an accept call like all the other servers
// Accept loop in the following routine
    chatLoop(fd_accept);

}
 


void chatLoop(int fd_accept)
{ 
    unsigned char buffer[BUFF_SIZE];    
    struct sockaddr_in fsin; 
    int fromlen;
    struct timeval stv; 
    int i; 
    int fd_server  = -1; 
    int iMaxFd, iNumInputs, nContinue = 1; 
    int newConnect, nread;
    fd_set fdCheck; 
    
    // The select statement is the key call in this routine. 
    
    for(;nContinue;) 
    { 
        stv.tv_sec = 0; 
        stv.tv_usec = 0; 
        FD_ZERO(&fdCheck); 

// Select mask contains accept socket number
        FD_SET(fd_accept  , &fdCheck); 
        iMaxFd = fd_accept; 
   
// Select mask contains socket number of each chat user's socket
        for (i = 0; i < numChats; i++)
        {
            FD_SET(fd_chats[i], &fdCheck);
            if (fd_chats[i] > iMaxFd) iMaxFd = fd_chats[i];
        }
        iMaxFd += 1; 
        
// No timeout on the select
        iNumInputs = select(iMaxFd, &fdCheck, 0, 0, NULL /* &stv*/); 
        if (iNumInputs == -1) 
        { 
            /* Error*/ 
            printf("\n Bad return from select ==> die\n"); 
            nContinue = 0; 
        } 
        else if (iNumInputs == 0) 
        { 
            /* Timeout -- won't occur for us.*/ 
            printf("\n Timeout from select\n"); 
            nContinue = 0;
        } 
        else 
        {
// Pending connect request of new chat user 
            if (FD_ISSET(fd_accept  , &fdCheck)) 
            { 
                 // Now we can make an accept without blocking
                 fromlen = sizeof(fsin); 
                 newConnect = accept(fd_accept, (struct sockaddr *)&fsin, &fromlen);
                 printf("New Connect : %d\n", newConnect);
                 if (newConnect < 0) 
                    { 
                        fprintf(stderr,"accept err: %s\n", strerror(errno));
                        continue; 
                    }                  

                 if (numChats >= MAX_CHATS)
                 {
                // We are full message sent
                    printf("%s\n", chatFull);
                    write(newConnect, chatFull, strlen(chatFull));
                    close(newConnect);
                 }
                 else
                    addChatter(newConnect); // add new user to tables
              
            }  // end of fd_accept handling
           
   
            for (i =0; i < numChats; i++)
            {
                if (FD_ISSET(fd_chats[i], &fdCheck))
                {
            // Received message from one of the chat clients
                    nread = read(fd_chats[i], buffer, BUFF_SIZE); 
                    if (nread > 0)
                    {
                        if (buffer[nread-1] == '\n') 
                            buffer[nread-1] = 0;
                        else
                            buffer[nread] = 0;

                    // processChat sends out information to all
                    // chatters
                        processChat(i,  buffer, nread);
                        
                    }
                    else
                    {
                        printf("Time to close out chat file descriptor: %d\n",
                            fd_chats[i]);
                        if (nread < 0)
                             fprintf(stderr, "Read err:%s\n", strerror(errno));
                        
                       // close out terminated chat user and compress the
                       // chat tables
                        remove_chatter(i);
                    }
                }
            } // end of for loop on numChats
        } 
    } // End of the select for loop    
}

//************************************************
// processChat called to deal with each chat message
 
void processChat(int instance, char * buffer, int nread)
{
   unsigned char outbuff[BUFF_SIZE]; 
   int i, len;
  
   len = strlen(change_name);
   if (strncmp(change_name, buffer, len)== 0)
    {        
    // Special message of  chatter requesting a name something like:
    // -name=Clem
    // In this case we copy Clem into the name_chats table

        strcpy((char *)&name_chats[instance], &buffer[len]);
        return;
    }
    
   // Prefix buffer with name of user who sent it
   len = sprintf(outbuff, "%s:\n%s\n", 
       (char *)&name_chats[instance], buffer);
   for (i=0; i < numChats; i++)
    {
    // Send buffer to all chatters
        write(fd_chats[i], outbuff, len);
    }
    
}

// *******************************************************
 // addChatter called to deal with a new Chatter connect

void addChatter(int newConnect)
{
  unsigned char buffer[BUFF_SIZE];  
  int i,total = 0;  

  sprintf(buffer,"unknown:%d", numChats);
  strcpy((char *)&name_chats[numChats], buffer);

// Create welcome message along with a list of who is currently
// connected to the chat server
 
  total = sprintf(buffer,"%s\n", welcome_msg);
  if (numChats <= 0)
    total +=sprintf(&buffer[total], "You are the first chatter\n");
  else
    {
        total += sprintf(&buffer[total], " Current Chatters\n");
        for ( i=0; i < numChats; i++)
        {
          total += sprintf(&buffer[total], "%s\n", (char *) &name_chats[i]);  
        }
    }

// Send message back to new connecting chatter
  write(newConnect, buffer, total);

// Add Chatter to fd_chat table
  fd_chats[numChats++] = newConnect;
}


void remove_chatter(int fd_toRemove)
{
   int k, fd_save; 
   // close out terminated chat user and compress the
   // chat tables
   fd_save = fd_chats[fd_toRemove];
   for (k=fd_toRemove; k < numChats -1; k++)
     {
        fd_chats[k] = fd_chats[k+1];
        strcpy((char *)&name_chats[k], (char *)&name_chats[k+1]);
     }
   numChats -= 1;
   printf("Closing %d\n", fd_save);
   close(fd_save);
}

