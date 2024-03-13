#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define KEY 1234
#define KEY2 1000
#define MAX_MTP_SOCK 25
#define IP_ADDRESS_MAX_LENGTH 46
#define SEND_WND 10
#define RECEIVE_WND 5
#define MAX_RECEIVE_BUFF 5*1024
#define MAX_SEND_BUFF 10*1024

typedef struct SOCK_INFO
{
    int sock_id;
    struct sockaddr_in IP;
    int port;
    int errorno;
} SOCK_INFO;

typedef struct send_window
{
    int size;
    char seq_nums[SEND_WND];
} send_window;

typedef struct receive_window
{
    int size;
    char seq_nums[RECEIVE_WND];
} receive_window;

typedef struct sharedMem
{
    int free;
    pid_t pid;
    int udp_sockid;
    int dest_port;
    struct sockaddr_in dest_IP;
    char receive_buff[MAX_RECEIVE_BUFF];
    char send_buff[MAX_SEND_BUFF];
    send_window swnd;
    receive_window rwnd;
    int arr[10];
} sharedMem;

sem_t sem1;
sem_t sem2;

void *R_func(void *arg)
{
    printf("R_func running\n");

    return NULL;
}

void *S_func(void *arg)
{
    printf("S_func running\n");
    return NULL;
}

int main()
{

    int shmid;
    SOCK_INFO *sockinfo;
    sharedMem *SM;

    shmid = shmget(KEY, sizeof(SOCK_INFO), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        printf("Unable to create shared mem\n");
    }

    sockinfo = (SOCK_INFO *)shmat(shmid, NULL, 0);
    if (sockinfo == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    sockinfo->errorno = 0;
    sockinfo->port = 0;
    sockinfo->sock_id = 0;
    sockinfo->IP.sin_addr.s_addr = INADDR_ANY;
    int SM_id;
    SM_id = shmget(KEY2, MAX_MTP_SOCK * sizeof(sharedMem), IPC_CREAT | 0666);
    if (SM_id == -1)
    {
        printf("Unable to create shared mem\n");
    }
    SM = (sharedMem *)shmat(SM_id, NULL, 0);
    if (SM == (void *)-1)
    {
        perror("shmat not working");
        exit(EXIT_FAILURE);
    }
    // SM[24].arr[24] = 100;
    // printf("output: %d\n", SM[24].arr[24]);

    pthread_t thread_R, thread_S;
    pthread_attr_t R_attr, S_attr;

    pthread_attr_init(&R_attr);
    pthread_attr_init(&S_attr);
    pthread_attr_setdetachstate(&R_attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setdetachstate(&S_attr, PTHREAD_CREATE_JOINABLE);

    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 0);
    pthread_create(&thread_R, &R_attr, R_func, NULL);
    pthread_create(&thread_S, &S_attr, S_func, NULL);
    // thread creation done

    while (1)
    {
        sem_wait(&sem1);
        inet_pton(AF_INET, "127.0.0.1", &sockinfo->IP.sin_addr.s_addr);
        sockinfo->sock_id = 1;
        sockinfo->port = htons(100);
        if (sockinfo->errorno == 0 && sockinfo->port == 0 && sockinfo->sock_id == 0 && sockinfo->IP.sin_addr.s_addr == INADDR_ANY)
        {
            // char ip_str[INET_ADDRSTRLEN];

            // inet_ntop(AF_INET, &sockinfo->IP.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
            printf("It is a UDP socket call\n");
            // printf("%d", sockinfo->IP.sin_addr.s_addr);
            // printf("%s", ip_str);
            int temp_sockid;
            if ((temp_sockid = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            {
                printf("error creating socket\n");
                sockinfo->sock_id = -1;
                sockinfo->errorno = errno;
            };
            sockinfo->sock_id = temp_sockid;
            sem_post(&sem2);
        }
        else if (sockinfo->sock_id != 0 && sockinfo->port != 0 && sockinfo->IP.sin_addr.s_addr != INADDR_ANY)
        {
            printf("It is a bind call\n");
            // printf("port: %d\n", sockinfo->port);
            struct sockaddr_in socket;
            socket.sin_family = AF_INET;
            socket.sin_addr.s_addr = sockinfo->IP.sin_addr.s_addr;
            socket.sin_port = htons(sockinfo->port);
            if ((bind(sockinfo->sock_id, (struct sockaddr *)&socket, sizeof(socket))) < 0)
            {
                printf("Error in binding\n");
                sockinfo->sock_id = -1;
                sockinfo->errorno = errno;
            }
            sem_post(&sem2);
        }
    }

    pthread_join(thread_R, NULL);
    pthread_join(thread_S, NULL);

    pthread_attr_destroy(&R_attr);
    pthread_attr_destroy(&S_attr);

    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return 0;
}