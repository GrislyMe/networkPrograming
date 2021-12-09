#include <arpa/inet.h>
#include <netdb.h>
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
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
char board[9];
int turn = 0;
char sign;

void help() {
	printf("for rename: 1 <Name>\n");
	printf("currently online player: 2\n");
	printf("invite player:3 <PlayerName>\n");
	printf("logout：logout\n");
	printf("help for help, simple：help\n\n");
}

void print_board(char* board) {
	printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");
	printf("│ 0 │ 1 │ 2 │        │ %c │ %c │ %c │\n", board[0], board[1], board[2]);
	printf("├───┼───┼───┤        ├───┼───┼───┤\n");
	printf("│ 3 │ 4 │ 5 │        │ %c │ %c │ %c │\n", board[3], board[4], board[5]);
	printf("├───┼───┼───┤        ├───┼───┼───┤\n");
	printf("│ 6 │ 7 │ 8 │        │ %c │ %c │ %c │\n", board[6], board[7], board[8]);
	printf("└───┴───┴───┘        └───┴───┴───┘\n");
}

// modify chess board, and fill "sendbuf" with package format.
void write_on_board(char* board, int location) {

	board[location] = sign;
	sprintf(sendbuf, "7  %c %c %c %c %c %c %c %c %c\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8]);
}

// Only handle message from server to client.
void pthread_recv(void* ptr) {
	int instruction;
	while (1) {
		memset(sendbuf, 0, sizeof(sendbuf));
		instruction = 0;
		// recvbuf is filled by server's fd.
		if ((recv(fd, recvbuf, MAXDATASIZE, 0)) == -1) {
			printf("recv() error\n");
			exit(1);
		}
		sscanf(recvbuf, "%d", &instruction);
		switch (instruction) {
		case 2: {
			printf("%s\n", &recvbuf[2]);  // Print the message behind the instruction.
			break;
		}
		case 4: {
			char inviter[100];
			sscanf(recvbuf, "%d %s", &instruction, inviter);
			printf("%s\n", &recvbuf[2]);  // Print the message behind the instruction.
			printf("for the invite, type 5 Y/N %s\n", inviter);
			break;
		}
		case 6: {
			char hoster[100];
			char dueler[100];
			sscanf(recvbuf, "6 %d %s %s\n", &turn, hoster, dueler);
			for (int i = 0; i < 9; i++)
				board[i] = '*';
			printf("game on\n");
			printf("* for the blank\n");
			printf("%s is O\n", hoster);
			printf("%s is X\n", dueler);
			printf("%s please make the first move\n", hoster);
			printf("type in -0 ~ -8 to make the move\n");
			print_board(board);
			if (turn)
				sign = 'O';
			else
				sign = 'X';

			break;
		}
		case 8: {

			char msg[100];
			sscanf(recvbuf, "%d %d %c %c %c %c %c %c %c %c %c %s", &instruction, &turn, &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8], msg);
			print_board(board);
			printf("%s\n", msg);
			if (turn != 2) {
				printf("type in -0 ~ -8 to make the move\n\n");
			} else {
				printf("////////game over/////////\n");
				printf("invite player to play again\n");
				help();
				sign = 0;
			}
			break;
		}

		default:
			break;
		}

		memset(recvbuf, 0, sizeof(recvbuf));
	}
}

int main(int argc, char* argv[]) {

	int numbytes;
	char buf[MAXDATASIZE];
	struct hostent* he;
	struct sockaddr_in server;
	turn = 2;
	sign = 0;
	if (argc != 2) {
		printf("Usage: %s <IP Address>\n", argv[0]);
		exit(1);
	}

	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	bzero(&server, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr = *((struct in_addr*)he->h_addr);

	if (connect(fd, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	// First, Add User.
	printf("connect success\n");
	char str[] = " have come in\n";
	printf("Pleace ENTER your user name：");
	fgets(name, sizeof(name), stdin);
	char package[100];
	sprintf(package, "1 %s", name);
	send(fd, package, (strlen(package)), 0);

	// how to use your program
	help();

	// Only handle message from server to client. (Goto pthread_recv finction)
	pthread_t tid;
	pthread_create(&tid, NULL, (void*)pthread_recv, NULL);

	// Only handle message from client to server.
	while (1) {
		memset(sendbuf, 0, sizeof(sendbuf));  // clear buf

		fgets(sendbuf, sizeof(sendbuf), stdin);  // Input instructions
		int location;

		if (sendbuf[0] == '-') {
			if (turn == 1) {
				sscanf(&sendbuf[1], "%d", &location);
				if (board[location] != '*')
					printf("The place is already exist!!\nchange a place!!!!!!\n");
				else
					write_on_board(board, location);
			} else if (turn == 0) {
				printf("It's not your turn!!\n");
				continue;
			} else if (turn == 2) {
				printf("You are not playing a game now!\n");
				continue;
			}
		}
		send(fd, sendbuf, (strlen(sendbuf)), 0);  // Send instructions to server
		if (strcmp(sendbuf, "help\n") == 0)
			help();

		// Logout
		if (strcmp(sendbuf, "logout\n") == 0) {
			memset(sendbuf, 0, sizeof(sendbuf));
			printf("You have Quit.\n");
			return 0;
		}
	}
	pthread_join(tid, NULL);
	close(fd);
	return 0;
}
