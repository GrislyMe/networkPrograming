#include "smol-shell/shell.h"
#include <pcap/pcap.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/ipv6.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define buff_size 256

int pipefd[2];
WINDOW* initCMD() {
	WINDOW* win = initscr();
	load_cmd("cmdlist");
	raw();
	noecho();
	start_color();
	init_color_pair();

	return win;
}

int print_package(const struct pcap_pkthdr* head, const u_char* data) {
	char buff[buff_size];
	// sprintf(buff, "time_sec: %08lX\n", head->ts.tv_sec);
	// write(pipefd[1], buff, buff_size);
	sprintf(buff, "time_stamp: %s", ctime((const time_t*)&head->ts.tv_sec));
	write(pipefd[1], buff, buff_size);

	struct ether_header* eth = (struct ether_header*)data;
	int type = ntohs(eth->ether_type);

	sprintf(buff, "Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2], eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
	write(pipefd[1], buff, buff_size);
	sprintf(buff, "Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2], eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);
	write(pipefd[1], buff, buff_size);

	sprintf(buff, "Types: %04X\n", type);
	write(pipefd[1], buff, buff_size);
	if (type == ETHERTYPE_IP || type == ETHERTYPE_IPV6) {
		struct iphdr* ip = (struct iphdr*)(data + sizeof(struct ether_header));
		sprintf(buff, "IP version: %d\n", ip->version);
		write(pipefd[1], buff, buff_size);
		if (ip->version == 6) {
			struct ipv6hdr* ip6 = (struct ipv6hdr*)(data + sizeof(struct ether_header));

			sprintf(buff, "Source IP: ");
			write(pipefd[1], buff, buff_size);
			for (int i = 0; i < 15; i++) {
				sprintf(buff, "%02X:", ip6->saddr.s6_addr[i]);
				write(pipefd[1], buff, buff_size);
			}
			sprintf(buff, "%02X\n", ip6->saddr.s6_addr[15]);
			write(pipefd[1], buff, buff_size);

			sprintf(buff, "Destination IP: ");
			write(pipefd[1], buff, buff_size);
			for (int i = 0; i < 15; i++) {
				sprintf(buff, "%02X:", ip6->daddr.s6_addr[i]);
				write(pipefd[1], buff, buff_size);
			}
			sprintf(buff, "%02X\n", ip6->daddr.s6_addr[15]);
			write(pipefd[1], buff, buff_size);
		} else {
			sprintf(buff, "Source IP: %s\n", inet_ntoa(*(struct in_addr*)(&ip->saddr)));
			write(pipefd[1], buff, buff_size);

			sprintf(buff, "Destination IP: %s\n", inet_ntoa(*(struct in_addr*)(&ip->daddr)));
			write(pipefd[1], buff, buff_size);
		}

		if (ip->protocol == 6) {
			struct tcphdr* tcp = (struct tcphdr*)(data + sizeof(struct ether_header) + sizeof(struct iphdr));
			sprintf(buff, "TCP ports:\n");
			write(pipefd[1], buff, buff_size);
			sprintf(buff, "---> Source: %d\n", ntohs(tcp->source));
			write(pipefd[1], buff, buff_size);
			sprintf(buff, "---> Destination: %d\n", ntohs(tcp->dest));
			write(pipefd[1], buff, buff_size);
		}

		if (ip->protocol == 17) {
			struct udphdr* udp = (struct udphdr*)(data + sizeof(struct ether_header) + sizeof(struct iphdr));
			sprintf(buff, "TCP ports:\n");
			write(pipefd[1], buff, buff_size);
			sprintf(buff, "---> Source: %d\n", ntohs(udp->source));
			write(pipefd[1], buff, buff_size);
			sprintf(buff, "---> Destination: %d\n", ntohs(udp->dest));
			write(pipefd[1], buff, buff_size);
		}
	}

	sprintf(buff, "=====================================\n");
	write(pipefd[1], buff, buff_size);
	return 0;
}

int read_pcap(char* path) {
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t* handle = pcap_open_offline(path, errbuf);
	if (!handle) {
		char ret[buff_size];
		sprintf(ret, "no such file %s\n", path);
		write(pipefd[1], ret, strlen(ret));
		return 1;  // for file open error
	}

	int total_amount = 0;
	int total_bytes = 0;
	char buff[buff_size];
	while (1) {
		struct pcap_pkthdr* head = NULL;
		const u_char* data = NULL;
		int ret = pcap_next_ex(handle, &head, &data);
		if (ret == 1) {
			total_amount++;
			total_bytes += head->caplen;
			print_package(head, data);
		} else {
			sprintf(buff, "No more packet from %s\n", path);
			write(pipefd[1], buff, buff_size);
			break;
		}
	}
	sprintf(buff, "Read: %d, byte: %d bytes\n", total_amount, total_bytes);
	write(pipefd[1], buff, buff_size);

	return 0;
}

int cmd_exec(char* input) {
	char* tok;
	tok = strtok(input, " ");
	if (!strcmp(input, "read")) {
		tok = strtok(NULL, " ");
		if (tok) {
			read_pcap(tok);
		}
	}

	return 0;
}

void signal_handler(int signum) {
	wait(NULL);
}

int main() {
	WINDOW* win = initCMD();
	pid_t pid = 0;
	char buff[buff_size];
	FILE* fd;
	fd = fopen("./log", "w+");

	while (1) {
		if (pipe(pipefd)) {
			addstr("pipe fiald\n");
			return 0;
		}
		attron(COLOR_PAIR(GREEN));
		addstr(header);
		attroff(COLOR_PAIR(GREEN));

		char input[buff_size];
		read_input(win, input);
		addch('\n');
		if (!strcmp(input, "exit"))
			return 0;

		pid = fork();
		if (pid == 0) {
			close(pipefd[0]);
			cmd_exec(input);
			return 0;
		} else {
			close(pipefd[1]);
			while (read(pipefd[0], buff, buff_size)) {
				fwrite(buff, 1, strlen(buff), fd);
				wprintw(win, "%s", buff);
			}
			signal(SIGCHLD, signal_handler);
			addch('\n');
		}
	}
	getch();
	endwin();
}
