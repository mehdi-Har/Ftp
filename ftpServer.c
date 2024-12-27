#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h> 
#define PORT 3490
#define BUFFER_SIZE 1024

void remove_directory(int client_fd, const char *dirname) {
    char *error_msg = "Error: could not remove directory\n";
    char *success_msg = "Directory removed successfully\n";

    if (rmdir(dirname) == 0) {
        send(client_fd, success_msg, strlen(success_msg), 0);
        printf("Directory '%s' removed successfully.\n", dirname);
    } else {
        send(client_fd, error_msg, strlen(error_msg), 0);
        perror("Directory removal failed");
    }
}

void change_directory(int client_fd, const char *path) {
    if (chdir(path) == 0) {
        char *success_msg = "Directory changed successfully\n";
        send(client_fd, success_msg, strlen(success_msg), 0);
        printf("Changed directory to '%s'.\n", path);
    } else {
        char *error_msg = "Error: failed to change directory\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        perror("Failed to change directory");
    }
}

void delete_file(int client_fd, const char *filename) {
    char *error_msg = "Error: file deletion failed\n";
    char *success_msg = "File deleted successfully\n";

    if (remove(filename) == 0) {
        send(client_fd, success_msg, strlen(success_msg), 0);
        printf("File '%s' deleted successfully.\n", filename);
    } else {
        send(client_fd, error_msg, strlen(error_msg), 0);
        perror("File deletion failed");
    }
}
void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void create_directory(int client_fd, const char *dirname) {
    char *error_msg = "Error: could not create directory\n";
    char *success_msg = "Directory created successfully\n";

    if (mkdir(dirname, 0755) == 0) {
        send(client_fd, success_msg, strlen(success_msg), 0);
        printf("Directory '%s' created successfully.\n", dirname);
    } else {
        send(client_fd, error_msg, strlen(error_msg), 0);
        perror("Directory creation failed");
    }
}
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        int valread = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[valread] = '\0';
        printf("Command received: %s\n", buffer);

        if (strcmp(buffer, "ls") == 0) {
            DIR *dir;
            struct dirent *entry;
            char file_list[BUFFER_SIZE] = {0};

            dir = opendir(".");
            if (dir == NULL) {
                perror("Directory open failed");
                strcpy(file_list, "Error opening directory\n");
            } else {
                while ((entry = readdir(dir)) != NULL) {
                    strcat(file_list, entry->d_name);
                    strcat(file_list, "\n");
                }
                closedir(dir);
            }

            send(client_fd, file_list, strlen(file_list), 0);
            printf("File list sent to client.\n");
        } else if (strncmp(buffer, "get ", 4) == 0) {
            char filename[BUFFER_SIZE];
            strcpy(filename, buffer + 4);
            filename[strcspn(filename, "\n")] = 0;

            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                char *error_msg = "Error: file not found\n";
                send(client_fd, error_msg, strlen(error_msg), 0);
            } else {
                char file_buffer[BUFFER_SIZE];
                int bytes_read;

                printf("Sending file: %s\n", filename);
                while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
                    send(client_fd, file_buffer, bytes_read, 0);
                }

                send(client_fd, "EOF", 3, 0);

                fclose(file);
                printf("File '%s' sent successfully.\n", filename);
            }
        } else if (strncmp(buffer, "put ", 4) == 0) {
            char filename[BUFFER_SIZE];
            strcpy(filename, buffer + 4);
            filename[strcspn(filename, "\n")] = 0;

            FILE *file = fopen(filename, "wb");
            if (file == NULL) {
                perror("File creation failed");
                char *error_msg = "Error: file creation failed\n";
                send(client_fd, error_msg, strlen(error_msg), 0);
                continue;
            }

            printf("Receiving file: %s\n", filename);
            char file_buffer[BUFFER_SIZE];
            int bytes_received;

            while ((bytes_received = recv(client_fd, file_buffer, BUFFER_SIZE, 0)) > 0) {
                if (bytes_received >= 3 && strncmp(file_buffer + bytes_received - 3, "EOF", 3) == 0) {
                    fwrite(file_buffer, 1, bytes_received - 3, file);
                    break;
                }
                fwrite(file_buffer, 1, bytes_received, file);
            }

            fclose(file);
            printf("File '%s' received successfully.\n", filename);

            char *success_msg = "File uploaded successfully.\n";
            send(client_fd, success_msg, strlen(success_msg), 0);
        } else if (strncmp(buffer, "DELE ", 5) == 0) {
            char filename[BUFFER_SIZE];
            strcpy(filename, buffer + 5);

            filename[strcspn(filename, "\n")] = '\0';  

            delete_file(client_fd , filename);
        } else if (strncmp(buffer , "MKD " , 4) == 0) {
            char dirname[BUFFER_SIZE];
            strcpy(dirname , buffer + 4);
            dirname[strcspn(dirname ,"\n")] = '\0';
            create_directory(client_fd , dirname);
            
        }
        else if (strncmp(buffer , "RMD " , 4) == 0) {
            char dirname[BUFFER_SIZE];
            strcpy(dirname , buffer + 4);
            dirname[strcspn(dirname ,"\n")] = '\0';
            remove_directory(client_fd , dirname);
            
        }else if (strncmp(buffer, "cd ", 3) == 0) { 
            char path[BUFFER_SIZE];
            strcpy(path, buffer + 3);
            path[strcspn(path, "\n")] = '\0'; 

            change_directory(client_fd, path);
        } 
         else if (strcmp(buffer, "exit") == 0) {
            printf("Client requested disconnection.\n");
            break;
        } else {
            char *msg = "Unsupported command\n";
            send(client_fd, msg, strlen(msg), 0);
        }
    }

    close(client_fd);
    exit(0); 
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    signal(SIGCHLD, sigchld_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Server socket created.\n");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), 0, 8);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server bound to port %d.\n", PORT);

    if (listen(server_fd, 5) < 0) {
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening...\n");

    while (1) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Connection acceptance failed");
            continue;
        }
        printf("Client connected from %s:%d.\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            close(client_fd);
            continue;
        }

       if (pid == 0) {  
    close(server_fd);  
    printf("Handling client in child process (PID: %d).\n", getpid());
    fflush(stdout); 
    handle_client(client_fd);
    exit(0); 
} else {  
            close(client_fd);  
        }
    }

    close(server_fd);
    return 0;
}
