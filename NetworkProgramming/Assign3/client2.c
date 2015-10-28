/************************************/
/* Client Side Code – Assignment 02 */
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
typedef struct sockaddr SA;
void str_cli (FILE *fp, int sockfd)
{
int n, maxfdp1, stdineof;
fd_set rset;
char sendline[MAXLINE], recvline[MAXLINE];
stdineof = 0;
FD_ZERO (&rset);
for ( ; ; )
{
if (stdineof == 0)
FD_SET (fileno (fp), &rset);
FD_SET (sockfd, &rset);
maxfdp1 = fileno (fp);
if(sockfd > maxfdp1)
{
maxfdp1 = sockfd;
}
maxfdp1 = maxfdp1 + 1;
select (maxfdp1, &rset, NULL, NULL, NULL);
if (FD_ISSET (sockfd, &rset))
{
/* socket is readable */
if ((n = read (sockfd, recvline, MAXLINE)) == 0)
{
if (stdineof == 1)
return;
/* normal termination */
else
{
perror ("str_cli: server terminated prematurely");
exit(0);
}
}
recvline[n] = 0;
fputs (recvline, stdout);
printf ("\n");
}
if (FD_ISSET (fileno (fp), &rset))
{
/* input is readable */
if (fgets (sendline, MAXLINE, fp) == NULL)
{
stdineof = 1;
shutdown (sockfd, SHUT_WR);
FD_CLR (fileno (fp), &rset);
continue;
/* send FIN */
}
sendline[strlen (sendline)-1] = 0;
if (write (sockfd, sendline, strlen (sendline)) < 0)
{
perror ("write error");
exit (0);
}
}
}
}
int main (int argc, char **argv)
{
int sockfd;
struct sockaddr_in servaddr;
if (argc != 2)
{
perror ("usage: tcpcli <IPaddress>");
exit (0);
}
sockfd = socket (AF_INET, SOCK_STREAM, 0);
if ( sockfd < 0)
{
perror ("socket error");
exit (0);
}
bzero (&servaddr, sizeof (servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_port = htons (SERV_PORT);
inet_pton (AF_INET, argv[1], &servaddr.sin_addr);
if (connect (sockfd, (SA *) &servaddr, sizeof (servaddr)) < 0)
{
perror ("connect error");
exit (0);
}
str_cli (stdin, sockfd);
exit (0);
}

