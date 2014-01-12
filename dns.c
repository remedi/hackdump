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
#include <unistd.h>

#include <time.h> // srand(time(NULL));

#define MAXDATASIZE 512
#define NAMESERVER "8.8.4.4"
#define PORT 53

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


/* Format for DNS header, creating struct with
each header field presented in uint16_t

 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                     ID                        |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    QDCOUNT                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ANCOUNT                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    NSCOUNT                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ARCOUNT                    |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/

struct header {
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
};


struct header* initHeader() {

	// Generating header with "random" id and recursion flag set
	struct header* h = malloc(sizeof(struct header));
	srand(time(NULL));
	h->id = rand();
	h->flags = htons(0x0100); // 0000 0001 0000 0000 QR = 0, RD = 1
	h->qdcount = htons(0x0001);
	h->ancount = 0;
	h->nscount = 0;
	h->arcount = 0;

	return h;
}

uint8_t* parseQuery(struct header* h, const char* address, size_t* length) {
	
	// Allocating memory for query
	size_t wHead = 12, segSize = 0;
	uint8_t* query = calloc((strlen(address)+16)*sizeof(uint8_t), 1);
	
	// First 12 bytes of query are the header
	memcpy(query, h, 12);

	for (size_t i = 0; address[i] != '\0' && i < 50;) {
		
		// Calculate the length of the segment
		segSize = 0;
		for (size_t j = i; address[j] != '\0' && address[j] != '.'; j++)
			segSize++;
		
		query[wHead] = segSize;
		wHead++;

		// Insert the ASCII characters into the query
		for (size_t j = i; j < segSize+i; j++) {
                        query[wHead] = address[j];
			wHead++;
                }	
		
		// Ending character 0x00
		if (address[i+segSize] == '\0') {
			query[wHead] = 0x00;
			query[wHead+1] = 0x00;
			query[wHead+2] = 0x01;
			query[wHead+3] = 0x00;
			query[wHead+4] = 0x01;
			break;
		}
		
		i += segSize+1;
	}

	*length = wHead+4;

	return query;
}

int makeConnection(const char* ip) {

	int newSocket;
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT);
	memset(sa.sin_zero, 0, 8);

	if (inet_pton(AF_INET, ip, &sa.sin_addr) < 1) {
		fprintf(stderr, "Invalid nameserver address");
		exit(EXIT_FAILURE);
	}

	if ((newSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Can't create socket\n");
		exit(EXIT_FAILURE);
	}

	if (connect(newSocket, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
		fprintf(stderr, "Can't connect\n");
		exit(EXIT_FAILURE);
	}

	return newSocket;
}

size_t sendQuestion(int socket, uint8_t* query, size_t length) {

	size_t sentBytes;
	sentBytes = send(socket, query, length+1, 0);

	if(sentBytes == length)
		return length;
	return sentBytes;
}

size_t receiveAnswer(int socket, uint8_t* answer) {

	size_t recvBytes;
	recvBytes = recv(socket, answer, MAXDATASIZE, 0);

	return recvBytes;
}

void printAddress(uint8_t* answer, const char* address, size_t bc) {

	printf("\nIP Address for host %s is ", address);
	for(size_t i = 12; i <= bc; i++)
		if (answer[i] == 0x00) {
			printf("%d.%d.%d.%d\n\n", answer[i+17], answer[i+18],
				answer[i+19], answer[i+20]);
			break;
		}
}

int main (int argc, const char* argv[]) {
	
	// Check if the user has not given enough parameters
	if (argc < 2) {
		printf("Usage: \"%s <address to resolve>\"\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Init structs and other variables
	int sock;
	struct header* h;
	uint8_t answer[MAXDATASIZE];
	size_t bc;
	uint8_t* query;
	

	printf("Initializing header...\n");
	h = initHeader();
	printf("Parsing query...");
	query = parseQuery(h, argv[1], &bc);
	printf("\n________________________________\nQuestion section:\n");
	for(int i = 0; i <= bc; i++) {
		printf("%02x", query[i]);
		if (i%2)
			printf(" ");
	}
	printf("\n________________________________\n");
	printf("Making connection...\n");	
	sock = makeConnection(NAMESERVER);
	
	printf("Sending data...\n");	
	bc = sendQuestion(sock, query, bc);
	printf("Sent %zu bytes.\n", bc);
	printf("Receiving data...\n");
	bc = receiveAnswer(sock, answer);
	printf("Received %zu bytes.", bc);

	printf("\n________________________________\nAnswer section:\n");
	for(int i = 0; i < bc; i++) {
		printf("%02x", answer[i]);
		if (i%2)
			printf(" ");
	}

	printf("\n________________________________\n");
	
	printAddress(answer, argv[1], bc);	

	close(sock);
	exit(EXIT_SUCCESS);
}
