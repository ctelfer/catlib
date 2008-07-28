#include <stdio.h>
#include <string.h>
#include <cat/err.h>
#include <cat/net.h>
#include <cat/io.h>

int main(int argc, char *argv[]) 
{ 
    int fd, n, n2; 
    char recvbuf[255]; 
    char sendbuf[255]; 
    char *host = "localhost";

    if ( argc > 1 )
        host = argv[1];

    DO(fd = tcp_cli(host, "10000")); 

    for ( ; ; ) { 

      if (fgets(sendbuf, 255, stdin) == NULL ) {close(fd); return 0; } 

      n = strlen(sendbuf); 
      DO(io_write(fd, sendbuf, n));

      DO(n2 = io_read(fd, recvbuf, n));
      recvbuf[n2] = '\0';
      fprintf(stdout, "Got back %d bytes : %s", n2, recvbuf);
    } 
  return 0; 
} 
