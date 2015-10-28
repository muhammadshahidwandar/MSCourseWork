/************************************/
/* Server Side Code – Assignment 02 */
/************************************/
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
#define MAXLINE 160
#define SERV_PORT 3300
#define LISTENQ 10
#define START_ID 1000
#define LOG_FILE "server_log.txt"



//////////////////////////////////////////////
#define BROADCAST_MSG 0              // The chat payload message should be
#define ERROR -3                     // This type could be used to tell about any
#define QUIT_CHAT -9                 // Any peer can send this message type to
#define USER_NAME -1                 // The chat payload contains the user name
#define PASSWD -2                    // The chat payload contains the password for
#define GET_USER_LIST -10            // This type tells the server to send the list
#define USER_NOT_FOUND -4           // User is not found. The client should send
#define USER_FOUND -5                // User is registered. Client should now send
#define PASSWD_INVALID -6            // password send for the registered user is
#define ASSIGNED_ID -7               // This packet contains the ID assigned to
#define WELCOME_MSG -8               // This message contains the welcome text from










typedef struct _ChatUserList
{
char chatUserName[MAXLINE];
char chatPassword[MAXLINE];
} ChatUserList;
typedef struct _ClientInformation
{
char cli_username[MAXLINE]; // User name of the client
int cli_chatUserListIndex;
// Index for the entry of this user
// in ChatUserList
int cli_id; // Assigned ID
int cli_sockfd; // Socket descriptor of this connection
int cli_status; // NotUsed=0; Online=1;
// WaitingForUserName=2;
// WaitingForPasswd=3;
} ClientInformation;


typedef struct sockaddr SA;
void sig_chld(int signo)
{
pid_t pid;
int stat;
printf ("about to terminate child\n");
pid = wait (&stat);
printf ("child %d terminated\n", pid);
return;
}
int main (int argc, char **argv)
{
int i, j, maxi, maxfd, listenfd, connfd, sockfd;
int nready, client[FD_SETSIZE], clientID[FD_SETSIZE];
ssize_t n;
fd_set rset, allset;
char line[MAXLINE];
char buffer[MAXLINE+10];
socklen_t clilen;
struct sockaddr_in cliaddr, servaddr;
int currentID = START_ID;
FILE* fptr;
if ((listenfd = socket (AF_INET, SOCK_STREAM, 0))< 0)
{
perror ("socket");
exit (0);
}
bzero (&servaddr, sizeof (servaddr));
servaddr.sin_family      = AF_INET;
servaddr.sin_addr.s_addr = htonl ((void *) &argv[1]); ///INADDR_ANY
servaddr.sin_port        = htons (SERV_PORT);
if (bind (listenfd, (SA *) &servaddr, sizeof (servaddr)) < 0)
{
perror ("bind");
exit (0);
}
listen (listenfd, LISTENQ);
signal (SIGCHLD, sig_chld);
maxfd = listenfd;
maxi = -1;
/* initialize */
/* index into client[] array */
for (i = 0; i < FD_SETSIZE; i++)
client[i] = -1;
/* -1 indicates available entry */
FD_ZERO (&allset);
FD_SET (listenfd, &allset);
for ( ; ; )
{
rset = allset;
/* structure assignment */
nready = select (maxfd+1, &rset, NULL, NULL, NULL);
if (nready < 0)
{
if (errno == EINTR)
{
continue;
}
else
{
printf ("select error\n");
exit (0);
}
}
if (FD_ISSET (listenfd, &rset))
{
/* new client connection */
clilen = sizeof (cliaddr);
connfd = accept (listenfd, (SA *) &cliaddr, &clilen);
if (connfd == -1)
{
perror ("accept");
exit (0);
}
if (fork() != 0)
{
for (i = 0; i < FD_SETSIZE; i++)
{
if (client[i] < 0)
{
client[i] = connfd;
/* save descriptor */
clientID[i] = currentID;
snprintf (buffer, MAXLINE+10, "WELCOME: YOUR CHATID IS %d", currentID);
write (connfd, buffer, strlen(buffer));
currentID++;
break;
}
}
if (i == FD_SETSIZE)
{
perror ("too many clients");
exit (0);
}
FD_SET (connfd, &allset);
/* add new descriptor to set */
if (connfd > maxfd)
maxfd = connfd;
/* for select */
if (i > maxi)
maxi = i;
/* max index in client[] array */
if (--nready <= 0)
continue;
/* no more readable descriptors */
}
else
{
close (connfd);
close (listenfd);
fptr = fopen (LOG_FILE, "a+");
if (fptr == NULL)
{
printf ("File open failed\n");
exit (0);
}
fprintf (fptr, "ID %d assigned to client with IP %s and Port %d\n", currentID, inet_ntoa (cliaddr.sin_addr),ntohs (cliaddr.sin_port));
printf("ID %d assigned to client with IP %s and Port %d\n",
currentID, inet_ntoa (cliaddr.sin_addr),
ntohs (cliaddr.sin_port));
fclose (fptr);
exit (0);
}
}
for (i = 0; i <= maxi; i++)
{
/* check all clients for data */
if ((sockfd = client[i]) < 0)
continue;
if (FD_ISSET (sockfd, &rset))
{
if ((n = read (sockfd, line, MAXLINE)) == 0)
{ /*4connection closed by client */
close (sockfd);
FD_CLR (sockfd, &allset);
client[i] = -1;
}
else
{
line[n] = 0;
snprintf(buffer, MAXLINE+10, "%d: %s", clientID[i], line);
printf ("%s\n", buffer);
for (j = 0; j <= maxi; j++)
{
if ((sockfd = client[j]) < 0)
continue;
write(sockfd, buffer, strlen (buffer));
}
}
if (--nready <= 0)
break;
}
}
}
}



