/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
 ***********************************/


#include "headsock.h"

#define BACKLOG 10

void str_ser(int sockfd, uint8_t error_probability);
// transmitting and receiving function
int send_ack(int sockfd, struct ack_so *ack, uint8_t error_probability);
uint8_t get_error_probability(char *arg);
void setServerAddress(struct sockaddr_in* my_addr);


//2nd param for error to simulate time out

int main(int argc, char **argv) {
    int sockfd, con_fd, ret;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    uint8_t error_probability = 0;

    pid_t pid;


    srand(time(NULL));

    if (argc > 2) {
        printf("parameters not match");
    }

    if (argc == 2) {
        error_probability = get_error_probability(argv[1]);
    }

    printf("Error Probability chosen is : %d\n", error_probability);

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //create socket
    if (sockfd < 0) {
        printf("error in socket!");
        exit(1);
    }

    setServerAddress(&my_addr);

    ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof (struct sockaddr)); //bind socket
    if (ret < 0) {
        printf("error in binding");
        exit(1);
    }

    ret = listen(sockfd, BACKLOG); //listen
    if (ret < 0) {
        printf("error in listening");
        exit(1);
    }

    while (1) {
        printf("waiting for data\n");
        sin_size = sizeof (struct sockaddr_in);
        con_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size); //accept the packet (blocking)
        // Return non negative on success, -1 on error
        // Params:
        //	sockfd: is a socket descriptor returned by the socket function.
        //	cliaddr is a pointer to struct sockaddr that contains client IP address and port.
        //	addrlen set it to sizeof(struct sockaddr).

        if (con_fd < 0) {
            printf("error in accept\n");
            exit(1);
        }

        if ((pid = fork()) == 0) // creat acception process, (return 0 at at child, process id at parent)
        {
            close(sockfd);
            str_ser(con_fd, error_probability); //receive packet and response
            close(con_fd);
            exit(0);
        } else close(con_fd); //parent process
    }
    close(sockfd);
    exit(0);
}

void str_ser(int sockfd, uint8_t error_probability) {
    char data_buffer[BUFSIZE];
    FILE *fp;
    struct pack_so *ptr_packet;
    char receive_buffer[PACK_SIZE];
    struct ack_so ack;
    int end = 0, bytes_received = 0, bytes_sent = 0, packets_received = 0, current_bytes_received = 0;
    long lseek = 0;

    ack.seq_num = NOT_SET;


    printf("receiving data!\n");

    while (!end) {
        packets_received++;
        printf("\n Packet # received %d \n", packets_received);

        bytes_received = 0;

        while (bytes_received != PACK_SIZE )
        {
            char current_receive_buffer[1];

            if ((current_bytes_received = recv(sockfd, &current_receive_buffer, 1, 0)) == -1) //receive the packet
            {
              printf("error when receiving\n");
              exit(1);
            }
            
            memcpy((receive_buffer + bytes_received), current_receive_buffer, current_bytes_received);

            bytes_received ++;


            
//            if(bytes_received != 0)
//                printf("bit is : %s\n", current_receive_buffer);

            //printf("bit is : %c\n", current_receive_buffer[0]);

           if (current_receive_buffer[0] == '\a')
           {


               printf("bit is : %d\n", current_bytes_received);
               printf("bit is : %c\n", current_receive_buffer[0]);


               printf("\ni am here\n");
               printf("bytes_received is %d\n", bytes_received);
//               sleep(5);
               //printf("i am here\n");
               
               break;
           }
        }
    

        ptr_packet = NULL;
        ptr_packet = (struct pack_so*) receive_buffer; // de-serialize 

        if (ack.seq_num == NOT_SET) {
            ack.seq_num = ptr_packet->seq_num; // set initial ack seq_num
        }


        printf("Byte Received is %d \n", bytes_received);
        printf("Received packet seq number is %d \n", ptr_packet->seq_num);
        printf("Expected seq number is %d \n", ack.seq_num);
        printf("Received packet length is %d \n", ptr_packet->len);

        
        printf("bytes received is %d \n", ptr_packet->len);

        ack.len = bytes_received - HEADLEN;


        if (ptr_packet->len != ack.len) // packet received length mismatched
        {

            printf("Packet received length mismatched\n");

            bytes_sent = send_ack(sockfd, &ack, error_probability);

        } else if (ptr_packet->seq_num != ack.seq_num) // duplicated received (not expected seq number)
        {
            printf("Sequence number mismatched\n");

            bytes_sent = send_ack(sockfd, &ack, error_probability);

        } else // received succeed
        {

            printf("Received Success\n");
            ack.len = ptr_packet->len;
            ack.seq_num = !ptr_packet->seq_num; // set next expected seq number

            bytes_sent = send_ack(sockfd, &ack, error_probability);


            if (receive_buffer[bytes_received - 1] == '\0') // eof 								//if it is the end of the file
            {
                end = 1;
                ptr_packet->len--;
            }

            memcpy((data_buffer + lseek), ptr_packet->data, ptr_packet->len);
            lseek += ptr_packet->len;
        }
    }

    if ((fp = fopen("myTCPreceive.txt", "wt")) == NULL) {
        printf("File doesn't exit\n");
        exit(0);
    }
    fwrite(data_buffer, 1, lseek, fp); //write data into file
    fclose(fp);
    printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int) lseek + 1);
}

int send_ack(int sockfd, struct ack_so *ack, uint8_t error_probability) {
    int byte_sent = 0;
    uint8_t timeout;

    int random = (rand() % 100 + 1); // ..100 >= error_probability

    if (error_probability >= random) {
        printf("Setting timeout error\n");
        timeout = TRUE;
    } else {
        printf("No delay, Random is %d and error is %d\n", random, error_probability);
        timeout = FALSE;

        printf("Sending ack\n");

        byte_sent = send(sockfd, ack, HEADLEN, 0);

        if (byte_sent == -1) {
            perror("Send error :");
            exit(1);
        }
    }

    return byte_sent;

}

uint8_t get_error_probability(char *arg) {
    int error_probability;

    if (!(sscanf(arg, "%d", &error_probability))) {
        perror("Unable to parse Error Probability : ");
        exit(1);
    }
    if (error_probability > 100 || error_probability < 0) {
        printf("Invalid range for Error Probability");
        exit(1);
    }

    return (uint8_t) error_probability;
}

void setServerAddress(struct sockaddr_in* my_addr) {
    my_addr->sin_family = AF_INET;
    my_addr->sin_port = htons(MYTCP_PORT);
    my_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(my_addr->sin_zero), 8);
}
