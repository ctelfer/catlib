#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cat/err.h>
#include <cat/net.h>
#include <cat/io.h>

int main(int argc, char *argv[]) 
{ 
int lfd, fd, n=1; 
struct sockaddr_in remaddr; 
socklen_t alen;
char recvbuf[256], strbuf[256]; 

  ERRCK(lfd = tcp_srv(NULL, "10000")); 
  alen = sizeof(remaddr);
  fd = accept(lfd, (SA *)&remaddr, &alen);

  net_tostr((SA *)&remaddr, strbuf, 255);
  fprintf(stderr, "Connection from %s\n", strbuf); 

  do 
  { 
    ERRCK(n = io_read_upto(fd, recvbuf, 255));
    recvbuf[n] = '\0';
    fprintf(stderr, "received %s", recvbuf); 
    if ( n > 0 )
      ERRCK(io_write(fd, recvbuf, n));
  } while (n > 0); 

  close(fd); 
  close(lfd);
  return 0; 
} 
