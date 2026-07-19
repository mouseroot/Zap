#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

// Helper function to serve files back to the client browser
void serve_file(int client_fd, const char *filename, const char *content_type) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        // Send a 404 Not Found response if file is missing
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        write(client_fd, not_found, strlen(not_found));
        return;
    }

    // Determine the exact size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file contents into a buffer
    char *file_buffer = malloc(file_size);
    if (!file_buffer) {
        fclose(file);
        return;
    }
    fread(file_buffer, 1, file_size, file);
    fclose(file);

    // Build and send the HTTP response headers
    char header_buffer[512];
    int header_len = snprintf(header_buffer, sizeof(header_buffer),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        content_type, file_size);

    write(client_fd, header_buffer, header_len); // Send headers
    write(client_fd, file_buffer, file_size);     // Send payload data

    free(file_buffer);
}

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("[Zap]\tBind failed: Port in use\n");
        return 1;
    }
    if (listen(server_fd, 10) < 0) { printf("[Zap]\tListen failed\n"); return 1; }
    printf("[Zap]\tlistening on port %d\n", PORT);

    // Main connection acceptance loop
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char request_buffer[1024] = {0};
        read(client_fd, request_buffer, sizeof(request_buffer) - 1);

        // Parse out the HTTP method and target path (e.g., "GET /style.css HTTP/1.1")
        char method[16], path[256];
        sscanf(request_buffer, "%15s %255s", method, path);
        printf("[Zap] %s\t%s\n", method, path);
        // Simple Router routing requests to assets based on path string
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) 
        {
            serve_file(client_fd, "html/index.html", "text/html");
        } 
        else if (strcmp(path, "/style.css") == 0) {
            serve_file(client_fd, "html/style.css", "text/css");
        } 
        else if (strcmp(path, "/app.js") == 0) {
            serve_file(client_fd, "html/app.js", "application/javascript");
        } 
        else if(strcmp(path, "/manifest.json") == 0) {
            serve_file(client_fd, "html/manifest.json","application/manifest");
        }
        else if(strcmp(path, "/icon_512.png") == 0) {
            serve_file(client_fd, "html/icon_512.png","image/png");
        }
        else if(strcmp(path, "/icon.png") == 0) {
            serve_file(client_fd, "html/icon.png","image/png");
        }
        else {
            // Unhandled endpoint fallback
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            write(client_fd, not_found, strlen(not_found));
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
