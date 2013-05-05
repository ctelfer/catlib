/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <cat/err.h>
#include <cat/net.h>

int main(int argc, char *argv[]) 
{ 
  int fd, n=1; 
  char recvbuf[255], aname[255]; 
  struct sockaddr_storage sas;
  socklen_t addrsiz = 255, slen = sizeof(sas);

  ERRCK(fd = udp_sock(NULL, "10000"));

  ERRCK(getsockname(fd, (SA *)&sas, &slen));
  fprintf(stderr, "listening on %s\n", net_tostr((SA *)&sas, aname, 255));
  
  while ( 1 ) 
  {
    n = recvfrom(fd, recvbuf, 255, 0, (SA *)&sas, &addrsiz); 
    fprintf(stderr, "Received connection from %s\n",
	    net_tostr((SA *)&sas, aname, 255));
    sendto(fd, recvbuf, n, 0, (SA *)&sas, addrsiz); 
  }

  close(fd); 
  return 0;
} 
