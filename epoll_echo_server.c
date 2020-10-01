#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define PORT 8081
#define MAX_CONNECTIONS 2000
char * get_datetime_for_log()
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char  * datetime = malloc(4096);
	snprintf(datetime, sizeof(datetime) * 8, "%d-%02d-%02d %02d:%02d:%02d", (tm.tm_year + 1900), (tm.tm_mon + 1), (tm.tm_mday), (tm.tm_hour), tm.tm_min , tm.tm_sec );
	return datetime;
}
void CERR(char * msg)
{
	char * datetime = get_datetime_for_log();
	printf("[%s] <ERROR> : %s\n", datetime, msg);
	free(datetime);
}
// Logging is turned off to minimize runtime
void CLOG(char * msg)
{
	char * datetime = get_datetime_for_log();
	printf("[%s] <LOG> : %s\n", datetime, msg);
	free(datetime);
}
//Report
void CREP(char * msg)
{
	char * datetime = get_datetime_for_log();
	printf("[%s] <REP> : %s\n", datetime, msg);
	free(datetime);
}
void set_nonblocking(int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
		CERR("fcntl F_GETFL");
	}

	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		CERR("fcntl F_SETFL O_NONBLOCK");
	}
}
void set_sockaddr(struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(PORT);
}
void epoll_loop(struct epoll_event *events, struct epoll_event *ev, int epollfd, int server_fd, clock_t * start)
{
	int i;
	int connections_count, new_socket_events, sock_conn_fd;
	char buffer[2048] = "\0";
	while (connections_count < MAX_CONNECTIONS) {
		// poll for new connections
		if ((new_socket_events = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1)) == -1) {
			CERR("epoll_wait failed");
			exit(EXIT_FAILURE);
		}
		// Start clock if it is the first connection so we get an accurate measurement
		if (connections_count == 0) {
			*start = clock();
		}
		// Loop through all newly polled connections
		for (i = 0; i < new_socket_events; ++i) {
			// If this connection is the server then accept new connection on the server
			if (events[i].data.fd == server_fd) {
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				sock_conn_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
				++connections_count;
				if (sock_conn_fd == -1) {
					CERR("accept new connection failed");
					exit(EXIT_FAILURE);
				} else {
					CLOG("accepted new connection");
				}
				// Set the new connection to nonblocking
				set_nonblocking(sock_conn_fd);
				// Set the events it should watch for and its file descriptor. In, edge, and close
				ev->events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP;
				ev->data.fd = sock_conn_fd;
				// Add to the epoll wathc list
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock_conn_fd, ev) == -1) {
					CERR("new event add failed");
				} else {
					CLOG("new event added");
				}
				// if the event is a read operation read and echo it directly back to the open file descriptor
			} else if (events[i].events & EPOLLIN) {
				memset(buffer, 0, sizeof buffer);
				int sock_fd = events[i].data.fd;
				ssize_t read_size;
				read_size = read(sock_fd, buffer, 2048);
				CLOG(buffer);
				int write_size = write(sock_fd, buffer, strlen(buffer));

			}
			// If the event is a close operation remove it from the epoll watch list and close the file descriptor
			if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
				if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
					CERR("close event failed");
				} else {
					char temp_buf[128];
					snprintf(temp_buf, sizeof(temp_buf), "event %d closed", events[i].data.fd);
					CLOG(temp_buf);
				}
				close(events[i].data.fd);
				continue;
			}
		}
	}
}
void run_server()
{
	//initialzing variables for sockets
	int server_fd;

	// Initializing variables for epoll
	struct epoll_event ev, events[MAX_CONNECTIONS];
	int  epollfd;

	// Start socket intialization
	// Create socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		CERR("socket failed");
		exit(EXIT_FAILURE);
	}
	//Set socket to nonblocking
	set_nonblocking(server_fd);
	struct sockaddr_in server_addr;
	set_sockaddr(&server_addr);

	// Bind the socket to port 8080 and check to see if it failed
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		CERR("bind failed");
		char temp_buf[128];
		snprintf(temp_buf, sizeof(temp_buf), "%d", errno);
		CERR(temp_buf);
		exit(EXIT_FAILURE);
	}
	// Start listening on port 8081 for connections and check to see if the listen failed
	if (listen(server_fd, MAX_CONNECTIONS) < 0) {
		CERR("listen failed");
		exit(EXIT_FAILURE);
	} else {
		char temp_buf[128];
		snprintf(temp_buf, sizeof(temp_buf), "server started and is listening at %d", PORT);
		CLOG(temp_buf);
	}

	// Start epoll initialization
	// Create epoll file descriptor

	if ((epollfd = epoll_create(MAX_CONNECTIONS)) < 0) {
		CERR("epoll_create failed");
		exit(EXIT_FAILURE);
	}
	// Add server to epoll watch list
	// Set server to read operations
	ev.data.fd = server_fd;
	ev.events = EPOLLIN;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
		CERR("listeing socket add to epoll failed");
		exit(EXIT_FAILURE);
	}
	//Initialize timer
	clock_t start, end;
	double cpu_time_used;
	// Loop until all connections for test have been made
	epoll_loop(events, &ev, epollfd, server_fd, &start);
	// Stop the timing test and print out how many ticks it took
	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	char temp_buf[128];
	snprintf(temp_buf, sizeof(temp_buf), "%d connections were made to the server", MAX_CONNECTIONS);
	CREP(temp_buf);
	snprintf(temp_buf, sizeof(temp_buf), "Server took %f", cpu_time_used);
	CREP(temp_buf);
}
int main(int argc, char const *argv[])
{
	run_server();
	return 0;
}