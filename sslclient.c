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

// Include OpenSSL libraries
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAXDATASIZE 64

/*
struct addrinfo {
	int ai_flags; // AI_PASSIVE, AI_CANONNAME, etc.
	int ai_family; // AF_INET, AF_INET6, AF_UNSPEC
	int ai_socktype; // SOCK_STREAM, SOCK_DGRAM
	int ai_protocol; // use 0 for "any"
	size_t ai_addrlen; // size of ai_addr in bytes
	struct sockaddr *ai_addr; // struct sockaddr_in or _in6
	char *ai_canonname; // full canonical hostname

	struct addrinfo *ai_next; // linked list, next node
};
struct sockaddr_in {
	short int sin_family; // Address family, AF_INET
	unsigned short int sin_port; // Port number
	struct in_addr sin_addr; // Internet address
	unsigned char sin_zero[8]; // Same size as struct sockaddr
};
struct sockaddr_in6 {
	u_int16_t sin6_family; // address family, AF_INET6
	u_int16_t sin6_port; // port number, Network Byte Order
	u_int32_t sin6_flowinfo; // IPv6 flow information
	struct in6_addr sin6_addr; // IPv6 address
	u_int32_t sin6_scope_id; // Scope ID
};
*/

int main(int argc, const char* argv[]) {
	
	if(argc != 4) {
		printf("Usage: %s <server> <port> <certificate>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Setting up buffer for receiving data
	char buf[MAXDATASIZE];
	memset(buf, 0, MAXDATASIZE*sizeof(char));

	// Init SSL
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	// Make connection
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

	// SSL
	BIO* b;
	char ipp[20];
	memset(ipp, 0, 20*sizeof(char));
	strcat(ipp, ipstr);
	strcat(ipp, ":");
	strcat(ipp, argv[2]);

	printf("Connecting to %s\n", ipp);

	SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
	SSL* ssl;

	b = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(b, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	if(!b)
		printf("Error loading bio\n");


	if(!SSL_CTX_load_verify_locations(ctx, argv[3], NULL)) {
		printf("Error loading trust store %s\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	// Connect SSL
	BIO_set_conn_hostname(b, ipp);
	if(BIO_do_connect(b) <= 0) {
		printf("Error with making SSL connection\n");
		exit(EXIT_FAILURE);
	}
	
	printf("Successful SSL connection\n");

	// Check the certificate
	if(SSL_get_verify_result(ssl) != X509_V_OK) {
		printf("Certificate not valid\n");
		exit(EXIT_FAILURE);
	}

	// Printing out the used cipher
	printf("SSL uses cipher: %s\n", SSL_CIPHER_get_name(SSL_get_current_cipher(ssl)));
	

	// Checking the commonName matches the hostname
	X509* p;
	char p_CN[256];
	p = SSL_get_peer_certificate(ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(p), NID_commonName, p_CN, 256);

	printf("commonName: %s", p_CN);

	if(!strcmp(argv[1], p_CN))
		printf("...OK\n");
	else {
		printf("...FAILED\n");
		exit(EXIT_FAILURE);
	}


	// Read validation code
	BIO_read(b, buf, 3);	

	printf("Validation code: %s\n", buf); 

	// Send validation:226143
	strcat(buf, ":226143");
	printf("Sending \"%s\"\n", buf);
	BIO_write(b, buf, 10);

	// Read response
	BIO_read(b, buf, MAXDATASIZE);

	printf("Received \"%s\"\n", buf);	

	// Free structures and close
	BIO_free_all(b);
	SSL_CTX_free(ctx);
	return 0;
}
