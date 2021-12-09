#include "util.h"

int main() {
	int server = create_socket("127.0.0.1", "http");
	int pid;

	while (1) {
		struct client_info* client = malloc(sizeof(client_info));

		client->socket = accept(server, (struct sockaddr*)&(client->address), &(client->address_length));
		if (client->socket < 0) {
			fprintf(stderr, "accept() failed.\n");
			continue;
		}
		printf("New connection from %s\n", get_client_address(client));

		if ((pid = fork()) < 0) {
			fprintf(stderr, "fork() failed.\n");
			return 1;
		}

		if (pid == 0) {
			// child
			close(server);
			connection_init(client);
			exit(0);
		} else {
			// parent
			drop_client(client);
		}
	}

	printf("\nClosing child socket...\n");

	printf("Finished.\n");
	return 0;
}
