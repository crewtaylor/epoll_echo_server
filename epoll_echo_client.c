#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#define PORT 8081
#define MAX_CONNECTIONS 2000

void *run_client(void * threadid);

char * get_datetime_for_log()
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char  * datetime = malloc(4096);
    snprintf(datetime, sizeof(datetime) * 8, "%d-%02d-%02d %02d:%02d:%02d", (tm.tm_year + 1900), (tm.tm_mon + 1), (tm.tm_mday), (tm.tm_hour), tm.tm_min , tm.tm_sec );
    return datetime;
}

void CLOG(char * level, char * msg)
{
    char * datetime = get_datetime_for_log();
    printf("[%s] <%s> : %s\n", datetime, level, msg);
    free(datetime);
}

int main(int argc, char const *argv[])
{
    int i = 0;
    pthread_t threads[MAX_CONNECTIONS];
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    for (i = 0; i < MAX_CONNECTIONS; ++i) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&threads[i], NULL, run_client, arg);
    }
    for (i = 0; i < MAX_CONNECTIONS; ++i) {
        pthread_join(threads[i], NULL);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    char temp_buf[128];
    snprintf(temp_buf, sizeof(temp_buf), "%d connections to the server were made", MAX_CONNECTIONS);
    CLOG("REPORT", temp_buf);
    snprintf(temp_buf, sizeof(temp_buf), "Client took %f", cpu_time_used);
    CLOG("REPORT", temp_buf);
    return 0;
}

void *run_client(void * threadid)
{
    int sock = 0;
    struct sockaddr_in server_address;
    char buffer[2048] = {0};
    int id = *((int *) threadid);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        CLOG("ERROR", "socket failed to create");
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        CLOG("ERROR", "address is invalid");
    }
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        CLOG("ERROR", "connection failed");
        printf("%d\n", errno);
    }

    char temp_buf[1024];
    snprintf(temp_buf, sizeof(temp_buf), "Hello from %d", id);
    CLOG("LOG", temp_buf);
    send(sock, temp_buf , strlen(temp_buf) , 0);
    int read_length = read(sock, buffer, 2048);
    close(sock);

    free(threadid);
    return 0;
}