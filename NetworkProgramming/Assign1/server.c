

    /* serverprog.c - a stream socket server demo */

    #include <stdio.h>

    #include <stdlib.h>

    #include <unistd.h>

    #include <errno.h>

    #include <string.h>

    #include <sys/types.h>

    #include <sys/socket.h>

    #include <netinet/in.h>

    #include <arpa/inet.h>

    #include <sys/wait.h>

    #include <signal.h>

/*char * sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
char portstr[7];
static char str[128];
switch(sa->sa_family) {
case AF_INET: {
struct sockaddr_in *sin = (struct sockaddr_in *) sa;
if(inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str))== NULL)
return (NULL);
if(ntohs(sin->sin_port) != 0)
{
snprintf(portstr, sizeof(portstr),“.%d”, ntohs(sin->sin_port))
strcat(str, portstr);
}
return (str);
}*/


     

    /* the port users will be connecting to */

    #define MYPORT 2400

    /* how many pending connections queue will hold */

    #define BACKLOG 1
    #define MAXLINE 100

  int main(int argc, char *argv[ ])

    {
      char buff[MAXLINE];


    /* listen on sock_fd, new connection on new_fd */

    int sockfd, new_fd;

    /* my address information */

    struct sockaddr_in my_addr;

    /* connector’s address information */

    struct sockaddr_in their_addr;
   // char aray [];

    int sin_size;

    int yes = 1;
 

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)

    {

        perror("Server-socket() error lol!");

        exit(1);

    }

    else

    printf("Server-socket() sockfd is OK...\n");
/* host byte order */
    my_addr.sin_family = AF_INET;

    /* short, network byte order */

    my_addr.sin_port = htons(MYPORT);

    /* automatically fill with my IP */
    my_addr.sin_addr.s_addr = INADDR_ANY;
  
    printf("Server-Using %s and port %d...\n", inet_ntoa(my_addr.sin_addr), MYPORT);

     

    /* zero the rest of the struct */

    memset(&(my_addr.sin_zero), '\0', 8);

     

    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)

    {

        perror("Server-bind() error");

        exit(1);

    }

    else

        printf("Server-bind() is OK...\n");

     

    if(listen(sockfd, BACKLOG) == -1)

    {

        perror("Server-listen() error");

        exit(1);

    }

    printf("Server-listen() is OK...Listening...\n");

     

    
    

    /* accept() loop */

    while(1)

    {

    sin_size = sizeof(struct sockaddr_in);

    if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)

    {

        perror("Server-accept() error");

        continue;

    }

    else

        printf("Server-accept() is OK...\n");

    printf("Server-new socket, new_fd is OK...\n");

    printf("Server: Got connection from %s\n", inet_ntoa(their_addr.sin_addr));

     

    /* this is the child process */

    if(!fork())

    {

        /* child doesn’t need the listener */

       close(sockfd);
       //aray  = inet_ntoa(their_addr.sin_addr);
       snprintf(buff, sizeof(buff), "WELCOM:%s\r\n",inet_ntoa(their_addr.sin_addr) ) ; /*sock_ntop(&their_addr, sizeof(their_addr)))*/;
      
       if(send(new_fd, buff,sizeof(buff), 0) == -1)
      // if(send(new_fd, "This is a test string from server!\n", 37, 0) == -1)

            perror("Server-send() error lol!");

       close(new_fd);

       exit(0);

    }

    else

        printf("Server-send is OK...!\n");

     

    /* parent doesn’t need this*/

    close(new_fd);

    printf("Server-new socket, new_fd closed successfully...\n");

    }

    return 0;

    }

