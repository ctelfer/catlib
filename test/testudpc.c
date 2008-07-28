#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cat/err.h>
#include <cat/net.h>

int main(int argc, char *argv[]) 
{ 
int fd, n; 
struct sockaddr_storage sas;
struct sockaddr_in *sin = (struct sockaddr_in *)&sas;
socklen_t remlen;
char buf[256], abuf[256]; 

  DO(fd = udp_sock(NULL, NULL));
  DO(net_resolv("localhost", "10000", NULL, &sas));


  for ( ; ; ) 
  { 
    if (fgets(buf, 255, stdin) == NULL ) 
    {
      close(fd); 
      return 1; 
    } 

    n = strlen(buf)+1; 
    remlen = sizeof(*sin);
    if ( sendto(fd, buf, n, 0, (SA *)sin, remlen) < 0 )
      errsys("sendto:\n\t");
    DO(n = recvfrom(fd, buf, 255, 0, (SA *)sin, &remlen)); 
    fprintf(stdout,"Received reply from %s\n",net_tostr((SA *)sin,abuf,256));
    fprintf(stdout, "Got back %d bytes : %s", n, buf);
  } 

  return 0; 
} 
