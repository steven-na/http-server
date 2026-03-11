#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
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

#define PORT 7001
#define BUFFER_SIZE 1024

i32 read_file_in(char **file_buffer, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        perror("file load"), exit(1);

    fseek(fp, 0L, SEEK_END);
    long lSize = ftell(fp);
    rewind(fp);

    *file_buffer = calloc(1, lSize + 1);
    if (!*file_buffer) {
        fclose(fp);
        perror("memory alloc fails");
        return -1;
    }

    if (1 != fread(*file_buffer, lSize, 1, fp)) {
        fclose(fp);
        free(*file_buffer);
        perror("entire read fails");
        return -1;
    }
    fclose(fp);
    return lSize + 1;
}

i32 replace_text(char **haystack, const char *needle, const char *replace) {
    size_t haystackLen = strlen(*haystack);
    char *pos = strstr(*haystack, needle);
    if (!pos) {
        return 0;
    }
    i32 offset = pos - *haystack;
    size_t ndlLen = strlen(needle);
    size_t repLen = strlen(replace);
    size_t tailLen = haystackLen - offset - ndlLen + 1;

    if (repLen > ndlLen) {
        char *tmp = realloc(*haystack, haystackLen + (repLen - ndlLen) + 1);
        if (!tmp)
            return -1;
        *haystack = tmp;
    }

    pos = *haystack + offset;
    memmove(pos + repLen, pos + ndlLen, tailLen);
    memcpy(pos, replace, repLen);

    return strlen(*haystack) + 1;
}

i32 replace_all_text(char **haystack, const char *needle, const char *replace) {
    size_t haystackLen = strlen(*haystack);
    for (;;) {
        char *pos = strstr(*haystack, needle);
        if (!pos)
            return 0;

        i32 offset = pos - *haystack;
        size_t ndlLen = strlen(needle);
        size_t repLen = strlen(replace);
        size_t tailLen = haystackLen - offset - ndlLen + 1;

        if (repLen > ndlLen) {
            char *tmp = realloc(*haystack, haystackLen + (repLen - ndlLen) + 1);
            if (!tmp)
                return -1;
            *haystack = tmp;
        }

        pos = *haystack + offset;
        memmove(pos + repLen, pos + ndlLen, tailLen);
        memcpy(pos, replace, repLen);
    }

    return strlen(*haystack) + 1;
}

typedef struct {
    char *title;
    char *author;
    char *date;
    char *fileName;
} post;

i32 get_posts_list(post **posts_list) {
    char *post_list;
    i32 file_size = read_file_in(&post_list, "./posts/posts.txt");
    if (file_size == -1) {
        perror("posts file read");
        return -1;
    }
    i32 post_count = 0;

    size_t cursor_pos = 0;
    for (;;) {
        char *line_start = post_list + cursor_pos;

        char *title_end_pos = strstr(line_start, " ");
        if (!title_end_pos)
            break;
        char *author_end_pos = strstr(title_end_pos + 1, " ");
        if (!author_end_pos)
            break;
        char *date_end_pos = strstr(author_end_pos + 1, " ");
        if (!date_end_pos)
            break;
        char *filename_end_pos = strstr(date_end_pos + 1, "\n");
        if (!filename_end_pos)
            break;

        size_t title_len = title_end_pos - line_start;
        size_t author_len = author_end_pos - (title_end_pos + 1);
        size_t date_len = date_end_pos - (author_end_pos + 1);
        size_t filename_len = filename_end_pos - (date_end_pos + 1);

        char *title = malloc(title_len + 1);
        char *author = malloc(author_len + 1);
        char *date = malloc(date_len + 1);
        char *filename = malloc(filename_len + 1);
        if (!title || !author || !date || !filename)
            return -1;

        memcpy(title, line_start, title_len);
        memcpy(author, title_end_pos + 1, author_len);
        memcpy(date, author_end_pos + 1, date_len);
        memcpy(filename, date_end_pos + 1, filename_len);

        title[title_len] = '\0';
        author[author_len] = '\0';
        date[date_len] = '\0';
        filename[filename_len] = '\0';

        replace_all_text(&title, "%20", " ");
        replace_all_text(&author, "%20", " ");
        replace_all_text(&date, "%20", " ");

        post *tmp = realloc(*posts_list, sizeof(post) * (post_count + 1));
        if (!tmp)
            return -1;
        *posts_list = tmp;

        (*posts_list)[post_count] = (post){.title = title,
                                           .author = author,
                                           .date = date,
                                           .fileName = filename};
        post_count++;
        cursor_pos = (filename_end_pos + 1) - post_list;
    }

    return post_count;
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

i32 recent_post_endpoint(char **resp_buffer, char *params) {
    post *posts_list = NULL; // initialize to NULL
    i32 post_count = get_posts_list(&posts_list);
    if (post_count <= 0 || !posts_list) {
        return 0;
    }
    const post *p = &posts_list[post_count - 1];
    char endpoint_resp[] = "HTTP/1.0 301 Moved Permanently\r\n"
                           "Location: /posts/%s\r\n"
                           "\r\n";
    char *redirect_resp;
    asprintf(&redirect_resp, endpoint_resp, p->fileName);
    *resp_buffer = calloc(1, strlen(redirect_resp) + 1);
    if (!*resp_buffer) {
        free(redirect_resp);
        perror("memory alloc fails");
        return -1;
    }
    strcpy(*resp_buffer, redirect_resp);
    return strlen(*resp_buffer) + 1;
}

i32 specific_post_endpoint(char **resp_buffer, char *params) {
    char *html_content;
    i32 lSize = read_file_in(&html_content, "./templates/post.html");
    if (lSize == -1) {
        perror("file read");
        return 0;
    }

    post *posts_list = NULL;
    i32 post_count = get_posts_list(&posts_list);
    if (post_count <= 0 || !posts_list) {
        return 0;
    }
    const post *p = NULL;
    int post_index;
    for (post_index = 0; post_index < post_count; post_index++) {
        if (strcmp(posts_list[post_index].fileName, params) == 0) {
            p = &posts_list[post_index];
            break;
        }
    }

    if (!p) {
        return 0;
    }

    printf("val: %i %i\n", post_index, post_count);
    if (post_index > 0) {
        const post *prev_post = &posts_list[post_index - 1];
        lSize = replace_text(&html_content, "<!-- PREVIOUS POST TITLE -->",
                             prev_post->title);
        lSize = replace_text(&html_content, "PREVIOUS-POST-SLUG",
                             prev_post->fileName);
    } else {
        const char *rem_prev_start =
            strstr(html_content, "<!-- REMOVE PREV POST START -->");
        if (!rem_prev_start) {
            return 0;
        }
        const char *rem_prev_end =
            strstr(html_content, "<!-- REMOVE PREV POST END -->");
        if (!rem_prev_end) {
            return 0;
        }

        i32 end_marker_len = strlen("<!-- REMOVE PREV POST END -->");
        i32 delete_offset = rem_prev_start - html_content;
        i32 delete_length = (rem_prev_end - rem_prev_start) + end_marker_len;
        i32 delete_tailln = lSize - delete_offset - delete_length + 1;

        printf("[DEBUG] bounds check: delete_offset(%i) + delete_length(%i) + "
               "delete_tailln(%i) = %i, lSize=%i\n",
               delete_offset, delete_length, delete_tailln,
               delete_offset + delete_length + delete_tailln, lSize);
        memmove(html_content + delete_offset,
                html_content + delete_offset + delete_length, delete_tailln);
        char *tmp = realloc(html_content, lSize - delete_length + 1);
        if (!tmp) {
            return 0;
        }
        lSize -= delete_length;
        html_content = tmp;
    }

    if ((post_index + 1) - post_count < 0) {
        const post *next_post = &posts_list[post_index + 1];
        lSize = replace_text(&html_content, "<!-- NEXT POST TITLE -->",
                             next_post->title);
        lSize =
            replace_text(&html_content, "NEXT-POST-SLUG", next_post->fileName);
    } else {
        const char *rem_next_start =
            strstr(html_content, "<!-- REMOVE NEXT POST START -->");
        if (!rem_next_start) {
            return 0;
        }
        const char *rem_next_end =
            strstr(html_content, "<!-- REMOVE NEXT POST END -->");
        if (!rem_next_end) {
            return 0;
        }

        i32 end_marker_len = strlen("<!-- REMOVE NEXT POST END -->");
        i32 delete_offset = rem_next_start - html_content;
        i32 delete_length = (rem_next_end - rem_next_start) + end_marker_len;
        i32 delete_tailln = lSize - delete_offset - delete_length + 1;

        printf("[DEBUG] bounds check: delete_offset(%i) + delete_length(%i) + "
               "delete_tailln(%i) = %i, lSize=%i\n",
               delete_offset, delete_length, delete_tailln,
               delete_offset + delete_length + delete_tailln, lSize);
        memmove(html_content + delete_offset,
                html_content + delete_offset + delete_length, delete_tailln);
        char *tmp = realloc(html_content, lSize - delete_length + 1);
        if (!tmp) {
            return 0;
        }
        lSize -= delete_length;
        html_content = tmp;
    }

    char *post_body;
    char *full_path;
    asprintf(&full_path, "./posts/%s", p->fileName);
    i32 post_size = read_file_in(&post_body, full_path);
    free(full_path);
    if (post_size == -1) {
        return 0;
    }

    replace_all_text(&html_content, "<!-- ARTICLE TITLE -->", p->title);
    replace_text(&html_content, "<!-- ARTICLE AUTHOR -->", p->author);
    replace_text(&html_content, "<!-- ARTICLE DATE -->", p->date);
    replace_text(&html_content, "<!-- ARTICLE CONTENT -->", post_body);

    i32 result =
        build_response(resp_buffer, html_content, strlen(html_content));
    if (result == -1) {
        return -1;
    }
    // free(html_content);
    return result;
}

i32 index_endpoint(char **resp_buffer, char *params) {
    char *html_content;
    i32 lSize = read_file_in(&html_content, "./pages/index.html");
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

i32 not_found_endpoint(char **resp_buffer) {
    char *html_content;
    i32 lSize = read_file_in(&html_content, "./pages/404.html");
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
    i32 (*func)(char **, char *);
    bool is_prefix;
} endpoint_mapping;

int main() {
    setbuf(stdout, NULL);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("sock create success.\n");
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
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

    u8 endpoint_count = 3;
    endpoint_mapping endpoints[endpoint_count];
    endpoints[0] = (endpoint_mapping){.endpoint = "/posts/recent/",
                                      .func = recent_post_endpoint,
                                      .is_prefix = false};
    endpoints[1] = (endpoint_mapping){.endpoint = "/posts/",
                                      .func = specific_post_endpoint,
                                      .is_prefix = true};
    endpoints[2] =
        (endpoint_mapping){.endpoint = "/home/", .func = index_endpoint};
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
                            "Location: /home/\r\n"
                            "\r\n";
            resp_len = strlen(endpoint_resp);
        } else {
            for (u8 i = 0; i < endpoint_count; i++) {
                printf("strncmp:%i\n", strncmp(uri, endpoints[i].endpoint,
                                               strlen(endpoints[i].endpoint)));
                if (endpoints[i].is_prefix) {
                    if (strncmp(uri, endpoints[i].endpoint,
                                strlen(endpoints[i].endpoint)) == 0) {
                        char *p = uri + strlen(endpoints[i].endpoint);
                        resp_len = endpoints[i].func(&endpoint_resp, p);
                        should_free = 1;
                        break;
                    }
                } else {
                    if (strcmp(uri, endpoints[i].endpoint) == 0) {
                        resp_len = endpoints[i].func(&endpoint_resp, "");
                        should_free = 1;
                        break;
                    }
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

    close(sockfd);

    return 0;
}
