# Epoll Echo Server Mini Project

### Usage

To run the code clone the repository and then cd into the directory `epoll_echo_server`.

```bash
~$ git clone https://github.com/crewtaylor/epoll_echo_server.git
~$ cd epoll_echo_server/
~/epoll_echo_server$ make
~/epoll_echo_server$ ./epoll_echo_server
```

Open a new terminal window

```bash
~$ cd epoll_echo_server/
~/epoll_echo_server$ ./epoll_echo_client
```

#### Server

The server is based on the _epoll_ event polling system in Linux. It supports **2000** connections until it closes and reports how long it took to read messages from all **2000** clients and echo back the messages. It has log ability for each connection event but this is turned off to optimize run time. If you would like logging uncomment the `CLOG` function.

#### Client

The client program spawns **2000** threads that all connect to the server and say hello with their `thread_id`. The threads then join together and the client program reports how long it took to send and receive the hello message. Logging is also turned off here to optimize for run time. If you would like logging uncomment the `CLOG` function.
