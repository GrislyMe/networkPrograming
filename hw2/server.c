#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 1234
#define BACKLOG 1
#define Max 10
#define MAXSIZE 1024

struct userinfo {
	char id[100];
	int inviting;
	int playwith;
};

int fdt[Max] = {0};
char mes[1024];
int SendToClient(int fd, char* buf, int Size);
struct userinfo users[100];
int find_fd(char* name);
int win_dis[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
void gameover(int sender, int targetfd);

void message_handler(char* mes, int sender) {
	int instruction = 0;
	sscanf(mes, "%d", &instruction);
	switch (instruction) {
	case 1: {  // add user
		char name[100];
		char buf[MAXSIZE];
		sscanf(mes, "1 %s\n", name);
		if (find_fd(name) != -1) {
			sprintf(buf, "2 %s name already taken, pain\n", name);
			send(sender, buf, strlen(buf), 0);
		} else {
			sscanf(&mes[2], "%s\n", users[sender].id);
			send(sender, "1", 1, 0);
			printf("1 chname:%d's id => %s\n", sender, name);
		}
		break;
	}
	case 2: {  // show
		char buf[MAXSIZE], tmp[100];
		int p = sprintf(buf, "2 ");
		for (int i = 0; i < 100; i++) {
			if (strcmp(users[i].id, "") != 0) {
				sscanf(users[i].id, "%s", tmp);
				if (users[i].playwith != -1)
					p = sprintf(buf + p, "%s is now play with %s\n", tmp, users[users[i].playwith].id) + p;
				else
					p = sprintf(buf + p, "%s is idle\n", tmp) + p;
			}
		}
		printf("2 show:%s\n", buf);

		send(sender, buf, strlen(buf), 0);

		break;
	}
	case 3: {  // invite
		char target[100];
		char buf[MAXSIZE];
		sscanf(mes, "3 %s\n", target);
		int targetfd = find_fd(target);
		if (users[sender].playwith != -1) {
			sprintf(buf, "2 you can't invite someone when you playing!\n");
			send(sender, buf, strlen(buf), 0);
		} else if (targetfd == -1) {
			sprintf(buf, "2 player is not online\ninput help to get how to use command\n");
			send(sender, buf, strlen(buf), 0);
		} else if (targetfd == sender) {
			sprintf(buf, "2 please don't duel with yourself...it's so tough\n");
			send(sender, buf, strlen(buf), 0);
		} else if (users[targetfd].playwith != -1) {
			sprintf(buf, "2 %s is now playing with other, use command 2 to check who is idle\n", users[targetfd].id);
			send(sender, buf, strlen(buf), 0);
		} else {
			sprintf(buf, "4 %s want invite you. Do you accept?\n", users[sender].id);
			users[sender].inviting = targetfd;
			send(targetfd, buf, strlen(buf), 0);
			printf("3 invite:%s to %s\n", users[sender].id, target);
		}
		break;
	}
	case 5: {  // agree(Y or y) or not(N or n)
		char state;
		char target[100];
		char buf[MAXSIZE];
		sscanf(mes, "5 %c %s\n", &state, target);
		if (strcmp(target, "") == 0) {
			sprintf(buf, "2 please enter the opponent's name!!\n");
			send(sender, buf, strlen(buf), 0);
			break;
		}
		int targetfd = find_fd(target);
		if (targetfd == -1) {
			sprintf(buf, "2 player is not online\n");
			send(sender, buf, strlen(buf), 0);
			break;
		}
		if (users[targetfd].inviting != sender) {
			sprintf(buf, "2 %s is not want duel with you now\n", users[targetfd].id);
			send(sender, buf, strlen(buf), 0);
			break;
		}
		if (state == 'Y' || state == 'y') {

			sprintf(buf, "6 0 %s %s\n", target, users[sender].id);
			send(sender, buf, strlen(buf), 0);
			sprintf(buf, "6 1 %s %s\n", target, users[sender].id);
			send(targetfd, buf, strlen(buf), 0);
			users[sender].playwith = targetfd;
			users[targetfd].playwith = sender;
			users[targetfd].inviting = -1;
			users[sender].inviting = -1;
			sprintf(buf, "2 remember! you are %s\n", users[sender].id);
			send(sender, buf, strlen(buf), 0);
			sprintf(buf, "2 remember! you are %s\n", users[targetfd].id);
			send(targetfd, buf, strlen(buf), 0);
			printf("6:\n");

		} else if (state == 'N' || state == 'n') {

			sprintf(buf, "2 You rejected %s 's inviation\n", users[targetfd].id);
			send(sender, buf, strlen(buf), 0);
			sprintf(buf, "2 %s is rejected yours inviation\n", users[sender].id);
			send(targetfd, buf, strlen(buf), 0);
			users[targetfd].inviting = -1;
			users[sender].inviting = -1;
		}
		break;
	}
	case 7: {

		char board[9];
		char state[100];
		char buf[MAXSIZE];
		sscanf(mes, "7  %c %c %c %c %c %c %c %c %c", &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8]);
		// read board in client

		for (int i = 0; i < 100; i++)
			state[i] = '\0';

		memset(buf, '\0', MAXSIZE);
		memset(state, '\0', sizeof(state));
		strcat(state, users[sender].id);

		// check win
		for (int i = 0; i < 8; i++) {
			if (board[win_dis[i][0]] == board[win_dis[i][1]] && board[win_dis[i][1]] == board[win_dis[i][2]]) {
				if (board[win_dis[i][0]] != '*') {
					strcat(state, "_Win!/////////////////\n");
					sprintf(buf, "8 2 %c %c %c %c %c %c %c %c %c %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);
					printf("7:%s", buf);
					send(sender, buf, sizeof(buf), 0);
					send(users[sender].playwith, buf, sizeof(buf), 0);
					gameover(sender, users[sender].playwith);
					return;
				}
			}
		}
		// no win
		// check draw
		memset(buf, '\0', MAXSIZE);
		memset(state, '\0', sizeof(state));
		for (int i = 0; i < 9; i++) {
			if (board[i] == '*')
				break;
			if (i == 8) {
				strcat(state, "////////draw!////////\n");
				sprintf(buf, "8 2 %c %c %c %c %c %c %c %c %c %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);
				printf("7:%s", buf);
				send(sender, buf, sizeof(buf), 0);
				send(users[sender].playwith, buf, sizeof(buf), 0);
				gameover(sender, users[sender].playwith);
				return;
			}
		}

		// no draw and no win, play continue
		memset(buf, '\0', MAXSIZE);
		memset(state, '\0', sizeof(state));
		strcat(state, users[users[sender].playwith].id);
		strcat(state, "_turn!\n");
		sprintf(buf, "8 1 %c %c %c %c %c %c %c %c %c %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);
		printf("7:%s", buf);
		send(users[sender].playwith, buf, sizeof(buf), 0);
		sprintf(buf, "8 0 %c %c %c %c %c %c %c %c %c %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);
		send(sender, buf, sizeof(buf), 0);
		break;
	}
	}
}

void* pthread_service(void* sfd) {
	int fd = *(int*)sfd;
	while (1) {
		int numbytes;
		int i;
		numbytes = recv(fd, mes, MAXSIZE, 0);
		printf("\n\n\n%s\n\n\n", mes);

		// close socket
		if (numbytes <= 0) {
			for (i = 0; i < Max; i++) {
				if (fd == fdt[i]) {
					fdt[i] = 0;
				}
			}
			memset(users[fd].id, '\0', sizeof(users[fd].id));
			users[fd].playwith = -1;
			break;
		}

		message_handler(mes, fd);
		bzero(mes, MAXSIZE);
	}
	close(fd);
}

int main() {
	int listenfd, connectfd;
	struct sockaddr_in server;
	struct sockaddr_in client;
	int sin_size;
	sin_size = sizeof(struct sockaddr_in);
	int number = 0;
	int fd;

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++)
			users[i].id[j] = '\0';
		users[i].playwith = -1;
		users[i].inviting = -1;
	}

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Creating socket failed.");
		exit(1);
	}

	int opt = SO_REUSEADDR;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&server, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(listenfd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1) {
		perror("Bind error.");
		exit(1);
	}

	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen() error\n");
		exit(1);
	}
	printf("Waiting for client....\n");

	while (1) {
		if ((fd = accept(listenfd, (struct sockaddr*)&client, &sin_size)) == -1) {
			perror("accept() error\n");
			exit(1);
		}

		if (number >= Max) {
			printf("no more client is allowed\n");
			close(fd);
		}

		int i;
		for (i = 0; i < Max; i++) {
			if (fdt[i] == 0) {
				fdt[i] = fd;
				break;
			}
		}
		pthread_t tid;
		pthread_create(&tid, NULL, (void*)pthread_service, &fd);
		number = number + 1;
	}
	close(listenfd);
}

int find_fd(char* name) {
	for (int i = 0; i < 100; i++)
		if (strcmp(name, users[i].id) == 0)
			return i;
	return -1;
}
void gameover(int sender, int targetfd) {

	users[sender].playwith = -1;
	users[targetfd].playwith = -1;
	users[sender].inviting = -1;
	users[targetfd].inviting = -1;
}
