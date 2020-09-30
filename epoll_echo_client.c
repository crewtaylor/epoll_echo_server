#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#define PORT 8081
#define MAX_CONNECTIONS 2000
void *run_client(void * threadid);
void CERR(char * msg)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%d-%02d-%02d %02d:%02d:%02d] <ERROR> : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
}
void CLOG(char * msg)
{
    // time_t t = time(NULL);
    // struct tm tm = *localtime(&t);
    // printf("[%d-%02d-%02d %02d:%02d:%02d] <SYS> : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
}
void CREP(char * msg)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%d-%02d-%02d %02d:%02d:%02d] <REP> : %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
}
int main(int argc, char const *argv[])
{
    pthread_t threads[MAX_CONNECTIONS];
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        pthread_create(&threads[i], NULL, run_client, (void *)&i);
    }
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        pthread_join(threads[i], NULL);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    char temp_buf[128];
    snprintf(temp_buf, sizeof(temp_buf), "%d connections to the server were made", MAX_CONNECTIONS);
    CREP(temp_buf);
    snprintf(temp_buf, sizeof(temp_buf), "Client took %f", cpu_time_used);
    CREP(temp_buf);
    return 0;
}
void *run_client(void * threadid)
{
    int sock = 0;
    struct sockaddr_in server_address;
    char buffer[2048] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        CERR("socket failed to create");
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        CERR("address is invalid");
    }
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        CERR("connection failed");
    }
    char temp_buf[1024];
    snprintf(temp_buf, sizeof(temp_buf), "Hello from %d", *(int *)threadid);
    send(sock, temp_buf , strlen(temp_buf) , 0);
    int read_length = read(sock, buffer, 2048);
    close(sock);
    CLOG(buffer);
    return 0;
}