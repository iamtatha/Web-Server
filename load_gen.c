/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

int time_up;
FILE *log_file;

// user info struct
struct user_info
{
    // user id
    int id;

    // socket info
    int portno;
    char *hostname;
    float think_time;

    // user metrics
    int total_count;
    float total_rtt;
};

// error handling function
void error(char *msg)
{
    // perror(msg);
}

// time diff in seconds
float time_diff(struct timeval *t2, struct timeval *t1)
{
    return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}





/*
------------------------------------------------------------------------------------------
--------------------------------- THREAD FUNCTION ----------------------------------------
------------------------------------------------------------------------------------------
*/





// user thread function
void *user_function(void *arg)
{
    /* get user info */
    struct user_info *info = (struct user_info *)arg;

    int sockfd, n;
    char buffer[256];
    struct timeval start, end;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    while (1)
    {
        /* start timer */
        gettimeofday(&start, NULL);

        /* TODO: create socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            // error("ERROR opening socket");
            break;
        }

        /* TODO: set server attrs */
        server = gethostbyname(info->hostname);
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
              server->h_length);
        serv_addr.sin_port = htons(info->portno);

        /* TODO: connect to server */
        int connection = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (connection < 0) {
            // error("ERROR connecting");
            break;
        }

        /* TODO: send message to server */
        strcpy(buffer, "GET /index.html HTTP/1.1");
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
            // error("ERROR writing to socket");
            break;
        }
        bzero(buffer, 256);

        /* TODO: read reply from server */
        n = read(sockfd, buffer, 255);
        if (n < 0) {
            // error("ERROR reading from socket");
            break;
        }

        /* TODO: close socket */
        close(sockfd);

        /* end timer */
        gettimeofday(&end, NULL);

        /* if time up, break */
        if (time_up)
            break;

        /* TODO: update user metrics */
        info->total_count++;
        info->total_rtt += time_diff(&end, &start);

        /* TODO: sleep for think time */
        // printf("\nThread %d | Count %d | rtt %f\n", info->id, info->total_count, time_diff(&end, &start));
        usleep(info->think_time * 1e6);
    }

    /* exit thread */
    fprintf(log_file, "User #%d finished\n", info->id);
    fflush(log_file);
    // printf("\nThread %d is exiting", info->id);
    pthread_exit(NULL);
}






/*
-----------------------------------------------------------------------------------------------------
----------------------------------------- MAIN FUNCTION ---------------------------------------------
-----------------------------------------------------------------------------------------------------
*/



int main(int argc, char *argv[])
{
    int user_count, portno, test_duration;
    float think_time;
    char *hostname;

    if (argc != 6)
    {
        fprintf(stderr,
                "Usage: %s <hostname> <server port> <number of concurrent users> "
                "<think time (in s)> <test duration (in s)>\n",
                argv[0]);
        exit(0);
    }

    hostname = argv[1];
    portno = atoi(argv[2]);
    user_count = atoi(argv[3]);
    think_time = atof(argv[4]);
    test_duration = atoi(argv[5]);

    printf("Hostname: %s\n", hostname);
    printf("Port: %d\n", portno);
    printf("User Count: %d\n", user_count);
    printf("Think Time: %f s\n", think_time);
    printf("Test Duration: %d s\n", test_duration);

    /* open log file */
    log_file = fopen("load_gen.log", "w");

    pthread_t threads[user_count];
    struct user_info info[user_count];
    struct timeval start, end;

    /* start timer */
    gettimeofday(&start, NULL);
    time_up = 0;
    for (int i = 0; i < user_count; ++i)
    {
        /* TODO: initialize user info */
        info[i].hostname = hostname;
        info[i].portno = portno;
        info[i].id = i + 1;
        info[i].think_time = think_time;
        info[i].total_count = 0;
        info[i].total_rtt = 0;

        /* TODO: create user thread */
        if(pthread_create(&threads[i], NULL, user_function, &info[i]) != 0) {
            error("Can't create thread");
        }

        fprintf(log_file, "Created thread %d\n", i);
    }

    printf("\nMain: Going to sleep | Threads will now take over\n");
    /* TODO: wait for test duration */
    sleep(test_duration);

    fprintf(log_file, "Woke up\n");

    /* end timer */
    time_up = 1;
    gettimeofday(&end, NULL);


    printf("\nMain: I am back | Threads joining will start\n");

    /* TODO: wait for all threads to finish */
    for (int i = 0; i < user_count; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    //printf("\nAll threads joined\n");

    
    /* close log file */
    fclose(log_file);

    /* TODO: print results */
    float avg_response_time, avg_throughput, total_rtt = 0, total_tc = 0;
    for (int i = 0; i < user_count; ++i)
    {
        total_rtt += info[i].total_rtt;
        total_tc += info[i].total_count;
    }
    avg_response_time = total_rtt / total_tc;
    avg_throughput = total_tc / (float)test_duration;

    printf("\nAvg Response time is: %f and Avg Throughput is: %f\n", avg_response_time, avg_throughput);
    
    return 0;
}
