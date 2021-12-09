#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_REQUEST_SIZE 2048
#define BUFSIZE 2048

typedef struct client_info {
	socklen_t address_length;
	struct sockaddr_storage address;
	int socket;
	char request[MAX_REQUEST_SIZE];
	int received;
	struct client_info* next;
} client_info;

int create_socket(const char* host, const char* port);

void drop_client(struct client_info* client);

const char* get_client_address(struct client_info* ci);

void send_400(struct client_info* client);

void send_404(struct client_info* client);

int connection_init(client_info* client);

int request_handler(client_info* client);
