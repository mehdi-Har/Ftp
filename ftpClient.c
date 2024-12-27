#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3490
#define BUFFER_SIZE 1024


void receive_file(int sockfd, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("File opening failed");
        return;
    }

    printf("Receiving file: %s\n", filename);
    int valread;
    while ((valread = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (valread >= 3 && strncmp(buffer + valread - 3, "EOF", 3) == 0) {
            fwrite(buffer, 1, valread - 3, file);
            break;
        }
        fwrite(buffer, 1, valread, file);
    }

    fclose(file);
    printf("File '%s' received successfully.\n", filename);
}

void send_file(int sockfd, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File opening failed");
        return;
    }

    printf("Uploading file: %s\n", filename);
    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(sockfd, buffer, bytes_read, 0);
    }
    send(sockfd, "EOF", 3, 0);
    fclose(file);
    printf("File '%s' uploaded successfully.\n", filename);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server.\n");

    while (1) {
        printf("ftp> ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; 

        if (strcmp(command, "exit") == 0) {
            printf("Exiting client.\n");
            break;
        }

        if (send(sockfd, command, strlen(command), 0) <= 0) {
            perror("Failed to send command");
            break;
        }

        if (strncmp(command, "get ", 4) == 0) {
            char filename[BUFFER_SIZE];
            strcpy(filename, command + 4); 
            receive_file(sockfd, filename);
        } else if (strncmp(command, "put ", 4) == 0) {
            char filename[BUFFER_SIZE];
            strcpy(filename, command + 4); 
            send_file(sockfd, filename);
            
        }
        else if (strncmp(command, "cd ", 3) == 0)
        {
            memset(buffer, 0, sizeof(buffer));
            int valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s\n", buffer);
            }

        }
        
        else if (strncmp(command, "DELE ", 5) == 0)
        {
            printf("Supreesion en cours \n");
            int valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s\n", buffer);
            }
        }
          else if (strncmp(command, "MKD ", 4) == 0)
        {
            printf("creation du directory en cours \n");
            int valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s\n", buffer);
            }
        }
           else if (strncmp(command, "RMD ", 4) == 0)
        {
            memset(buffer , 0 , sizeof(buffer));
            int valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s\n", buffer);
            }
        }
        else {

            int valread = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (valread > 0) {
                buffer[valread] = '\0'; 
                printf("%s\n", buffer);
            } else if (valread == 0) {
                printf("Server disconnected.\n");
                break;
            } else {
                perror("Failed to receive response");
                break;
            }
        }
    }

    
    close(sockfd);
    printf("Client shutdown.\n");
    return 0;
}
