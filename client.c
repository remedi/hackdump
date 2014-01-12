#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAXDATASIZE 100

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


int main (int argc, const char* argv[]) {
	
	// Check if the user has not given enough parameters
	if (argc != 3) {
		printf("Invalid usage. Use \"client www.address.com 8080\" instead.\n");
		exit(EXIT_FAILURE);
	}
	
	// Init structs and other variables
	struct addrinfo hints, *results, *i;
	int status;
	char ipstr[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	
	// Try to get addrinfo, exiting on error
	if ((status = getaddrinfo(argv[1], argv[2], &hints, &results)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	
	printf("IP addresses for %s:\n", argv[1]);
		
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
		printf(" %s: %s\n\n", ipver, ipstr);	
	}
	
	int sockfd, nbytes;
	char buf[MAXDATASIZE];
	
	printf("Connecting to server...\n");
	
	for (i = results; i != NULL; i = i->ai_next) { 
		if ((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1) {
			perror("socket error");
			continue;
		}
		if (connect(sockfd, i->ai_addr, i->ai_addrlen) == -1 ){
			close(sockfd);
			perror("connect error");
			continue;
		}
		break;
	}
	
	// Done with the addrinfo structure
	freeaddrinfo(results);
	
	if (i == NULL) {
		fprintf(stderr, "Failed to connect.");
		return 2;
	}
	
	// Receive data
	if ((nbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv error");
		exit(1);
	}
	
	buf[nbytes] = '\0';
	printf("Received: %s\n", buf);
	
	strcat(buf, ":226143\0");
	
	printf("Data about to be sent: %s\n", buf);
	
	// Send data
	if (send(sockfd, buf, 10, 0) == -1) {
		perror("send error");
		exit(1);
	}
	
	// Receive data
	if ((nbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv error");
		exit(1);
	}
	
	buf[nbytes] = '\0';
	printf("Received: %s\n", buf);
	
	// Close the socket after the sending and receiving is done
	close(sockfd);
	
	return 0;
}
