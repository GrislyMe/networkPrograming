#include "util.h"

int create_socket(const char* host, const char* port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* bind_address;
	if (getaddrinfo(NULL, "http", &hints, &bind_address) != 0) {
		fprintf(stderr, "get address fail. (%s)\n", gai_strerror(errno));
		exit(1);
	}

	int socket_listen;
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	// setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, NULL, sizeof(int));
	if (socket_listen < 0) {
		fprintf(stderr, "socket() failed. (%s)\n", gai_strerror(errno));
		exit(1);
	}

	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen) < 0) {
		fprintf(stderr, "bind() failed. (%s)\n", gai_strerror(errno));
		exit(1);
	}
	freeaddrinfo(bind_address);

	if (listen(socket_listen, 64) < 0) {
		fprintf(stderr, "listen() failed. (%s)\n", gai_strerror(errno));
		exit(1);
	}
	return socket_listen;
}

void drop_client(struct client_info* client) {
	close(client->socket);
}

const char* get_client_address(struct client_info* ci) {
	static char address_buffer[100];
	getnameinfo((struct sockaddr*)&ci->address, ci->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
	return address_buffer;
}

void send_400(client_info* client) {
	const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
	                   "Connection: close\r\n"
	                   "Content-Length: 11\r\n\r\nBad Request";
	send(client->socket, c400, strlen(c400), 0);
	drop_client(client);
}

void send_404(client_info* client) {
	const char* c404 = "HTTP/1.1 404 Not Found\r\n"
	                   "Connection: close\r\n"
	                   "Content-Length: 9\r\n\r\nNot Found";
	send(client->socket, c404, strlen(c404), 0);
	drop_client(client);
}

void send_200(client_info* client, const char* type, unsigned long size) {
	char buffer[BUFSIZE];
	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	write(client->socket, buffer, strlen(buffer));

	sprintf(buffer, "Connection: close\r\n");
	write(client->socket, buffer, strlen(buffer));

	if (size > 0) {
		sprintf(buffer, "Content-Length: %lu\r\n", size);
		write(client->socket, buffer, strlen(buffer));

		sprintf(buffer, "Content-Type: %s\r\n", type);
		write(client->socket, buffer, strlen(buffer));
	}
	sprintf(buffer, "\r\n");
	write(client->socket, buffer, strlen(buffer));
}

const char* get_content_type(const char* path) {
	const char* last_dot = strrchr(path, '.');
	if (last_dot) {
		if (strcmp(last_dot, ".css") == 0)
			return "text/css";
		if (strcmp(last_dot, ".csv") == 0)
			return "text/csv";
		if (strcmp(last_dot, ".gif") == 0)
			return "image/gif";
		if (strcmp(last_dot, ".htm") == 0)
			return "text/html";
		if (strcmp(last_dot, ".html") == 0)
			return "text/html";
		if (strcmp(last_dot, ".ico") == 0)
			return "image/x-icon";
		if (strcmp(last_dot, ".jpeg") == 0)
			return "image/jpeg";
		if (strcmp(last_dot, ".jpg") == 0)
			return "image/jpeg";
		if (strcmp(last_dot, ".js") == 0)
			return "application/javascript";
		if (strcmp(last_dot, ".json") == 0)
			return "application/json";
		if (strcmp(last_dot, ".png") == 0)
			return "image/png";
		if (strcmp(last_dot, ".pdf") == 0)
			return "application/pdf";
		if (strcmp(last_dot, ".svg") == 0)
			return "image/svg+xml";
		if (strcmp(last_dot, ".txt") == 0)
			return "text/plain";
	}

	return "application/octet-stream";
}

int get_req(client_info* client, char* filepath) {
	char path[128];
	strcat(path, filepath);
	if (strcmp(filepath, "/") == 0)
		sprintf(path, "./index.html");
	else
		sprintf(path, ".%s", filepath);

	printf("req path: %s\n", path);
	if (strstr(path, "..")) {
		send_400(client);
		return 1;
	}

	FILE* fp = fopen(path, "rb");
	if (!fp) {
		send_404(client);
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	size_t cl = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	const char* ct = get_content_type(path);

	char buffer[BUFSIZE];
	send_200(client, ct, cl);

	int r = fread(buffer, 1, BUFSIZE, fp);
	while (r) {
		write(client->socket, buffer, r);
		r = fread(buffer, 1, BUFSIZE, fp);
	}

	fclose(fp);
	return 0;
}

char* post_filename(char* request) {
	char tmp[BUFSIZE];
	strcpy(tmp, request);
	char* filename = strstr(tmp, "filename=\"");
	strtok(filename, "\"");
	filename = strtok(NULL, "\"");
	return filename;
}

int post_req(client_info* client, char* filepath) {
	client->received = read(client->socket, client->request, MAX_REQUEST_SIZE);
	char* filename = post_filename(client->request);
	char* data = strstr(client->request, "\r\n\r\n");

	if (!data) {
		fprintf(stderr, "failed to cut CRLF\n");
		return -1;
	}

	char dir[BUFSIZE];
	printf("uploading %s ...\n", filename);
	sprintf(dir, "./upload/%s", filename);
	int download = open(dir, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRWXO | S_IRWXU | S_IRWXG);
	if (download < 0) {
		fprintf(stderr, "unable to open file\n");
		return 0;
	}

	if (write(download, data, sizeof(*data)) < 0) {
		fprintf(stderr, "write error. %s(%d)\n", gai_strerror(errno), errno);
		close(download);
		return -1;
	}
	// printf("%s\n", data);

	long long int si = 0;
	while (1) {
		client->received = read(client->socket, client->request, MAX_REQUEST_SIZE);
		si += client->received;
		fprintf(stdout, "%lld\n", si);
		if (client->received == 0)
			break;

		if (write(download, client->request, client->received) < 1) {
			fprintf(stderr, "while write error.\n");
			close(download);
			return -1;
		}
	}

	printf("upload success\n");
	close(download);

	get_req(client, dir + 1);

	return 0;
}

int request_handler(client_info* client) {
	char* req = strtok(client->request, " ");
	printf("request [%s] from %s\n", req, get_client_address(client));
	if (!strcmp(req, "GET"))
		get_req(client, strtok(NULL, " "));
	else if (!strcmp(req, "POST"))
		post_req(client, strtok(NULL, " "));

	return 0;
}

int connection_init(client_info* client) {
	client->received = read(client->socket, client->request, MAX_REQUEST_SIZE);

	if (client->received < 1)
		fprintf(stderr, "Unexpected disconnect from %s.\n", get_client_address(client));
	else
		request_handler(client);
	drop_client(client);
	return 0;
}
