#include <unistd.h>
#include <stdio.h>
#include <cat/err.h>
#include <cat/net.h>

int main(int argc, char *argv[]) 
{ 
  int fd, n=1; 
  char recvbuf[255], aname[255]; 
  struct sockaddr_in sin;
  socklen_t addrsiz = 255, slen = sizeof(sin);

  ERRCK(fd = udp_sock(NULL, "10000"));

  ERRCK(getsockname(fd, (SA *)&sin, &slen));
  fprintf(stderr, "listening on %s\n", net_tostr((SA *)&sin, aname, 255));
  
  while ( 1 ) 
  {
    n = recvfrom(fd, recvbuf, 255, 0, (SA *)&sin, &addrsiz); 
    fprintf(stderr, "Received connection from %s\n",
	    net_tostr((SA *)&sin, aname, 255));
    sendto(fd, recvbuf, n, 0, (SA *)&sin, addrsiz); 
  }

  close(fd); 
  return 0;
} 
