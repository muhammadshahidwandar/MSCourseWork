/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define MYPORT "3300"    // the port users will be connecting to
#define MAXLINE 160

#define MAXBUFLEN 100
#define START_ID 1000
#define LOG_FILE "server_log.txt"
typedef struct sockaddr SA;


void sig_chld(int signo)
{
  pid_t pid;
  int stat;
  printf("about to terminate the child\n");
  //SIGCHLD = 0;
  pid = wait(&stat);
  printf("child %d terminated \n", pid);
  return;

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd ,i,j;
    FILE* fptr;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_in6 their_addr,ClientAddrArry[10];
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

   char line[MAXLINE+10];
     char buffer[MAXLINE];

    struct in6_addr ClientAddrTemp,ClientAddr[10];
    u_int16_t ClientPortTemp,ClientPort[10];
    int ClientId[10];
    int NumClients = 0;
    int CurrntId = START_ID; 
    for(i=0;i<10;i++)
       ClientId[i]=-1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    signal (SIGCHLD, sig_chld);
      
while(1) {
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    ClientAddrTemp= their_addr.sin6_addr;
    ClientPortTemp= their_addr.sin6_port;
  if(numbytes>=0)
 {
   for(i = 0; i<10;i++)
   {
      if(ClientId[i]==-1)
       {
         printf("its a new client %d\n ",i );
         ClientAddr[i]  = ClientAddrTemp;
         ClientPort[i]  = ClientPortTemp;
         ClientId[i]    = CurrntId;
         ClientAddrArry[i] = their_addr;
         snprintf (buffer, MAXLINE+10, "WELCOME: YOUR CHATID IS %d", CurrntId);
         printf("%s\n",buffer);
         sendto(sockfd,buffer,sizeof buffer,0,(struct sockaddr *)&their_addr,addr_len);
         // (connfd, buffer, strlen(buffer));
         if(fork() ==0)
         {
             fptr = fopen (LOG_FILE, "a+");
             if (fptr == NULL)
           {
              printf ("File open failed\n");
               exit (0);
           }
          fprintf (fptr, "ID= %d IP= %s Port= %d\n", CurrntId, inet_ntop(their_addr.sin6_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s),ClientPortTemp);
        printf("Server: got packet from %s\n",inet_ntop(their_addr.sin6_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
          fclose (fptr);
          exit (0);
         }
          CurrntId++;
         NumClients++;
          break;
       }

      else if((memcmp(&ClientAddr[i],&ClientAddrTemp,sizeof ClientAddrTemp)==0)&&(memcmp(&ClientPort[i],&ClientPortTemp,sizeof ClientPortTemp)==0))
        {
       
  //  printf("Server: got packet from %s\n",inet_ntop(their_addr.sin6_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
   for(j=0;j<NumClients;j++)
    {
   // printf("send buff data %s\n",buf);
    buf[numbytes] = '\0';
    snprintf (line, MAXLINE+10, "%d: %s", ClientId[i], buf);
    printf("%s\n",line);
    if ((numbytes = sendto(sockfd, line, sizeof line, 0,                         ///argv[2], strlen(argv[2]
            (struct sockaddr *)&ClientAddrArry[j] , sizeof ClientAddrArry[j])) == -1) {
             perror("talker: sendto");
             exit(1);                           }
   // printf("listener: packet contains \"%s\"\n", buf);
         }
      //   }
           break;
         }
      else
        {}

    } 
  }
}
    close(sockfd);

    return 0;
}
