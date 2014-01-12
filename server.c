#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAXDATASIZE 100
#define PORT "9990"
#define BACKLOG 10

/*
struct addrinfo {
	int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, etc.
	int                 ai_family;      // AF_INET, AF_INET6, AF_UNSPEC
	int                 ai_socktype;    // SOCK_STREAM, SOCK_DGRAM
	int                 ai_protocol;    // use 0 for "any"
	size_t              ai_addrlen;     // size of ai_addr in bytes
	struct sockaddr     *ai_addr;       // struct sockaddr_in or _in6
	char                *ai_canonname;  // full canonical hostname

	struct addrinfo	    *ai_next;       // linked list, next node
};
 
struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};
 
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
}; 

*/
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}


int main (void) {
	
	// Init structs and other variables
	struct addrinfo hints, *results, *i;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	int status;
	char ipstr[INET6_ADDRSTRLEN];
	char* msg = "Connection accepted. Usage:\nADD 0xN1,0xN2\nEXIT to close connection\n";
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	// Try to get addrinfo, exiting on error
	if ((status = getaddrinfo(NULL, PORT, &hints, &results)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	
	printf("IP addresses for localhost\n");
		
	for (i = results; i != NULL; i = i->ai_next) {
		void* addr;
		char* ipver;
		
		if (i->ai_family == AF_INET6) {
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) i->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		
		else {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*) i->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		
		inet_ntop(i->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
		printf(" %s: %s\n", ipver, ipstr);	
	}
	
	int sockfd, new_fd;
	// Bind to the first address found
	for (i = results; i != NULL; i = i->ai_next) {
		if((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1) {
			perror("socket error");
			continue;
		}
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt error");
			exit(1);
		}
		if(bind(sockfd, i->ai_addr, i->ai_addrlen) == -1) {
			perror("bind error");
			continue;
		}
		break;
	}
	
	if (i == NULL) {
		printf("Could not bind");
		return 2;
	}
	
	// Done with addrinfo structure
	freeaddrinfo(results);
	
	if(listen(sockfd, BACKLOG) == -1) {
		perror("listen error");
		exit(1);
	}
	
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	
	printf("Waiting for connections\n");
	
	while (1) {
		sin_size = sizeof(struct sockaddr_storage);
		if((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == -1) {
			perror("accept error");
			continue;
		}
		if (((struct sockaddr*)&their_addr)->sa_family == AF_INET)
			inet_ntop(their_addr.ss_family,  &((struct sockaddr_in*)&their_addr)->sin_addr, ipstr, sizeof ipstr);
		else
			inet_ntop(their_addr.ss_family, &((struct sockaddr_in6*)&their_addr)->sin6_addr, ipstr, sizeof ipstr);
			
		printf("Connection accepted from %s\n", ipstr);
		
		if (!fork()) {
			char buf[MAXDATASIZE];
			char buf2[MAXDATASIZE];
			int nbytes;
			uint n1, n2;
			close(sockfd);
			while(1) {
				if ((nbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1)
					perror("recv");
				buf[nbytes] = '\0';
				printf("Received %s\n", buf);
				
				// In case of HELLO
				if (strncmp(buf, "HELLO", 4) == 0)
					if (send(new_fd, msg, strlen(msg), 0) == -1)
						perror("send");
				
				// In case of ADD
				if (strncmp(buf, "ADD", 3) == 0) {
					nbytes = sscanf(buf, "ADD %x,%x", &n1, &n2);
					if (nbytes != 2)
						continue;
					n1 = n1 + n2;
					sprintf(buf2, "The sum is: %x\n", n1);
					if (send(new_fd, buf2, strlen(buf2), 0) == -1)
						perror("send");
				}
				
				// In case of EXIT
				if (strncmp(buf, "EXIT", 4) == 0) {
					printf("Closing connection\n");
					break;
				}
			}
			close(new_fd);
			exit(0);
        }
        close(new_fd);
		
	}
	
	// Close the socket after the sending and receiving is done
	close(sockfd);
	
	return 0;
}

