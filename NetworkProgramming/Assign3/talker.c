/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
//#define POLLRDNORM 4
#define OPEN_MAX 2
#define INFTIM 5
#define MAXLINE 160


#define MAXBUF 10*1024
#define SERVERPORT "3300"    // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    char buf[MAXBUF];
    int n_read,nready,maxi;
    struct pollfd client[OPEN_MAX-1];
    struct addrinfo hints, *servinfo, *p;
    char sendline[MAXLINE], recvline[MAXLINE];

    int rv;
    int numbytes;

    if (argc != 2) {
        fprintf(stderr,"usage: talker hostname/IP\n");
        exit(1);
                  }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
  
        

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) 
       {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
                                        }
       // printf("break \n");
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
        }
   // printf("Now lets get sstarted\n");
    client[0].fd   = sockfd;
    client[1].fd   = fileno(stdin);
    client[0].events= POLLRDNORM;
    client[1].events= POLLIN;
   // nready=poll(client,maxi+1,INFTIM);
   if ((numbytes = sendto(sockfd, NULL, 0, 0,                         ///argv[2], strlen(argv[2]
             p->ai_addr, p->ai_addrlen)) == -1) {
             perror("talker: sendto");
                                                }
   printf("zero byte sent\n");
   
    while((*sendline !=26))
{

    nready=poll(client,2,INFTIM);
   if(nready>0) {
   // printf("nread is here %d\n",nready);
                }
    if(client[1].revents&POLLIN)
     {  if (fgets (sendline, MAXLINE,stdin) == NULL)
        {  shutdown (sockfd, SHUT_WR);
           continue;
            }
        else  {
            sendline[strlen (sendline)-1] = 0;
            if ((numbytes = sendto(sockfd, sendline,strlen(sendline), 0,                         ///argv[2], strlen(argv[2]
             p->ai_addr, p->ai_addrlen)) == -1) {
             perror("talker: sendto");
             exit(1);                           }
          // printf("Recieved Data: %s\n",sendline);
             
       }
   }
  else if(client[0].revents&POLLRDNORM)
  {
   // printf("else if entered\n");
    n_read = recvfrom(sockfd,buf,MAXBUF,0,NULL,NULL);
  if (n_read<0) {
    perror("Problem in recvfrom");
    exit(1);
                }
    printf("%s\n",buf);
   // freeaddrinfo(servinfo);

  }
}
close(sockfd);
return 0;
}

