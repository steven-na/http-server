#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define PORT 80
#define BUFFER_SIZE 1024

i32 read_html_in(char **html_buffer, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        perror("file load"), exit(1);

    fseek(fp, 0L, SEEK_END);
    long lSize = ftell(fp);
    rewind(fp);

    *html_buffer = calloc(1, lSize + 1);
    if (!*html_buffer) {
        fclose(fp);
        perror("memory alloc fails");
        return -1;
    }

    if (1 != fread(*html_buffer, lSize, 1, fp)) {
        fclose(fp);
        free(*html_buffer);
        perror("entire read fails");
        return -1;
    }
    fclose(fp);
    return lSize + 1;
}

i32 replace_text(char *haystack, const char *needle, const char *replace) {
    size_t haystackLen = strlen(haystack);
    char *pos = strstr(haystack, needle);
    if (!pos) {
        return -1;
    }
    i32 offset = pos - haystack;

    size_t ndlLen = strlen(needle);
    size_t repLen = strlen(replace);
    size_t tailLen = haystackLen - offset - ndlLen;

    if (repLen > ndlLen) {
        haystack = realloc(haystack, haystackLen + (repLen - ndlLen) + 1);
        pos = haystack + offset;
    }
    memmove(pos + repLen, pos + ndlLen, tailLen);
    memcpy(pos, replace, repLen);
    haystackLen += (repLen - ndlLen);
    return haystackLen;
}

i32 build_response(char **resp_buffer, char *html_content, i32 html_size) {
    const char resp[] = "HTTP/1.0 200 OK\r\n"
                        "Content-type: text/html\r\n\r\n";

    *resp_buffer = calloc(1, strlen(resp) + html_size + 1);
    if (!*resp_buffer) {
        free(html_content);
        perror("memory alloc fails");
        return -1;
    }
    strcpy(*resp_buffer, resp);
    memcpy(*resp_buffer + strlen(resp), html_content, html_size);
    free(html_content);
    return strlen(resp) + html_size + 1;
}

i32 index_endpoint(char **resp_buffer) {
    char *html_content;
    i32 lSize = read_html_in(&html_content, "./pages/index.html");
    if (lSize == -1) {
        perror("file read");
        return -1;
    }

    i32 result = build_response(resp_buffer, html_content, lSize);
    if (result == -1) {
        return -1;
    }
    return result;
}

i32 post_endpoint(char **resp_buffer) {
    char *html_content;
    i32 lSize = read_html_in(&html_content, "./templates/post.html");
    if (lSize == -1) {
        perror("file read");
        return -1;
    }

    lSize = replace_text(html_content, "<!-- ARTICLE TITLE -->", "Test Title");
    lSize = replace_text(html_content, "<!-- ARTICLE TITLE -->", "Test Title");
    lSize =
        replace_text(html_content, "<!-- ARTICLE AUTHOR -->", "Test Author");
    lSize = replace_text(html_content, "<!-- ARTICLE DATE -->", "Jan 1, 2000");
    lSize = replace_text(html_content, "<!-- ARTICLE CONTENT -->",
                         "<p>Test Content</p>\n");

    i32 result = build_response(resp_buffer, html_content, lSize);
    if (result == -1) {
        return -1;
    }
    return result;
}

i32 not_found_endpoint(char **resp_buffer) {
    char *html_content;
    i32 lSize = read_html_in(&html_content, "./templates/404.html");
    if (lSize == -1) {
        perror("file read");
        return -1;
    }

    const char resp[] = "HTTP/1.0 404 Not Found\r\n"
                        "Content-type: text/html\r\n\r\n";

    *resp_buffer = calloc(1, strlen(resp) + lSize + 1);
    if (!*resp_buffer) {
        free(html_content);
        perror("memory alloc fails");
        return -1;
    }
    strcpy(*resp_buffer, resp);
    memcpy(*resp_buffer + strlen(resp), html_content, lSize);
    free(html_content);
    return strlen(resp) + lSize + 1;
};

typedef struct {
    char *endpoint;
    i32 (*func)(char **);
} endpoint_mapping;

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("sock create success.\n");

    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(PORT);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }
    printf("sock bind success..\n");

    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }
    printf("server listening for connectons...\n");
    char read_buffer[BUFFER_SIZE];

    u8 endpoint_count = 2;
    endpoint_mapping endpoints[endpoint_count];
    endpoints[0] =
        (endpoint_mapping){.endpoint = "/posts/", .func = post_endpoint};
    endpoints[1] =
        (endpoint_mapping){.endpoint = "/home", .func = index_endpoint};
    i32 (*nf_endpoint)(char **) = not_found_endpoint;

    for (;;) {
        int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }

        int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr,
                                (socklen_t *)&client_addrlen);
        if (sockn < 0) {
            perror("webserver (getsockname)");
            continue;
        }

        if (read(newsockfd, read_buffer, BUFFER_SIZE) < 0) {
            perror("webserver (read)");
            continue;
        }

        char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
        sscanf(read_buffer, "%s %s %s", method, uri, version);
        printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), method, version, uri);

        char *endpoint_resp;
        i32 resp_len = 0;
        i32 should_free = 0;

        if (strcmp(uri, "/") == 0) {
            endpoint_resp = "HTTP/1.0 301 Moved Permanently\r\n"
                            "Location: /home/\r\n";
            resp_len = strlen(endpoint_resp);
        } else {
            for (u8 i = 0; i < endpoint_count; i++) {
                printf("strncmp:%i\n", strncmp(uri, endpoints[i].endpoint,
                                               strlen(endpoints[i].endpoint)));
                if (strncmp(uri, endpoints[i].endpoint,
                            strlen(endpoints[i].endpoint)) == 0) {
                    resp_len = endpoints[i].func(&endpoint_resp);
                    printf("%i\n", resp_len);
                    should_free = 1;
                    break;
                }
            }
        }
        if (resp_len == 0) {
            resp_len = nf_endpoint(&endpoint_resp);
            should_free = 1;
        }

        if (write(newsockfd, endpoint_resp, resp_len) < 0) {
            perror("webserver (write)");
            continue;
        }

        if (should_free) {
            free(endpoint_resp);
        }

        printf("server accepted connection\n");

        close(newsockfd);
    }

    return 0;
}
