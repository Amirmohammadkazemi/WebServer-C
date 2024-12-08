#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_request(int client_socket);
void send_response(int client_socket, const char *status, const char *content_type, const char *body);
void send_file(int client_socket, const char *filename);
void send_404(int client_socket);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error opening socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding");
        exit(1);
    }

    listen(server_socket, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
        if (client_socket < 0) {
            perror("Error accepting connection");
            continue;
        }
        handle_request(client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    char method[10], path[255], version[10];

    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Error reading request");
        return;
    }

    buffer[bytes_read] = '\0';
    sscanf(buffer, "%s %s %s", method, path, version);

    printf("Request: %s %s %s\n", method, path, version);

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0) {
            strcpy(path, "/index.html");
        }

        char filepath[255];
        snprintf(filepath, sizeof(filepath), ".%s", path);

        if (access(filepath, F_OK) == 0) {
            send_file(client_socket, filepath);
        } else {
            send_404(client_socket);
        }
    } else {
        send_404(client_socket);
    }
}

void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];

    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, content_type, strlen(body), body);

    write(client_socket, response, strlen(response));
}

void send_file(int client_socket, const char *filename) {
    int file = open(filename, O_RDONLY);
    if (file < 0) {
        send_404(client_socket);
        return;
    }

    char file_content[BUFFER_SIZE];
    int bytes_read = read(file, file_content, sizeof(file_content) - 1);
    if (bytes_read < 0) {
        perror("Error reading file");
        send_404(client_socket);
        close(file);
        return;
    }

    file_content[bytes_read] = '\0'; 
    send_response(client_socket, "200 OK", "text/html", file_content);
    close(file);}

void send_404(int client_socket) {
    const char *body = "<html><body><h1>404 Not Found</h1></body></html>";
    send_response(client_socket, "404 Not Found", "text/html", body);
}

