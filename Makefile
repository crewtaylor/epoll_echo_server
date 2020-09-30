all:
	gcc -O3 epoll_echo_server.c -o epoll_echo_server
	gcc -pthread -O3 epoll_echo_client.c -o epoll_echo_client
clean:
	rm epoll_echo_server epoll_echo_client