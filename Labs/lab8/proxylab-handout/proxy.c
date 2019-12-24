#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

typedef struct WOBJ{
    char content[MAX_OBJECT_SIZE];
    char url[MAXLINE];
    struct WOBJ *next;
    int empty;
} web_obj;

typedef struct {
    web_obj *head, *tail;
    sem_t mutex, w;
    int read_cnt;
    int siz;
} Cache;

Cache cache;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *host, char *path, char *port);
void build_request_header(char *host, char *path, char *port, rio_t *rio, char *header);
void *thread(void *vargp);
void init_cache();
web_obj *Reader(char *url);
void Writer(web_obj *new_obj);

int main(int argc, char const *argv[])
{
    int listenfd, *connfd;
    static char port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;
    
    if(argc != 2){
        fprintf(stderr,"usage :%s <port> \n",argv[0]);
        exit(1);
    }

    strcpy(port, argv[1]);

    listenfd = Open_listenfd(port);
    init_cache();

    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("server connected to (%s:%s)\n", client_hostname, client_port);
        
        pthread_create(&tid, NULL, thread, connfd);
    }

    return 0;
}

void doit(int fd){
    static char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    static char host[MAXLINE], path[MAXLINE], header[MAXLINE], port[MAXLINE];
    int clientfd;
    
    rio_t rio, sio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s%s%s", method, uri, version);

    if(strcmp(method, "GET")){
        fprintf(stderr, "The tiny doesn't implement this method");
        exit(0);
    }

//    read_requesthdrs(&rio);
    if(!parse_uri(uri, host, path, port)){
        fprintf(stderr, "Uri: %s could not be parsed", uri);
        exit(0);
    }
    
    build_request_header(host, path, port, &rio, header);

/*
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&sio, clientfd);
    Rio_writen(clientfd, header, strlen(header));

    size_t n; 

    while((n = Rio_readlineb(&sio, buf, MAXLINE)) != 0) {
	    printf("Proxy server received %d bytes\n", (int)n);
	    Rio_writen(fd, buf, n);
    }

    close(clientfd);
*/
    char *url;
    url = Malloc(sizeof(host) + sizeof(path));
    strcpy(url, host);
    strcat(url, path);

    web_obj *page; 
    page = Reader(url);
    free(url);

    if(page -> empty){
        clientfd = Open_clientfd(host, port);
        Rio_readinitb(&sio, clientfd);
        Rio_writen(clientfd, header, strlen(header));

        size_t n, totalsize = 0;
        int already_removed = 0;

        while((n = Rio_readlineb(&sio, buf, MAXLINE)) != 0) {
            totalsize += n;

	        printf("Proxy server received %d bytes\n", (int)n);
	        Rio_writen(fd, buf, n);
            
            if(totalsize <= MAX_OBJECT_SIZE){
                strcat(page -> content, buf);
                page -> empty = 0;
            }
            else if(!already_removed){
                memset(page -> content, 0, sizeof(page -> content));
                memset(page -> url, 0, sizeof(page -> url));
                page -> empty = 1;
                already_removed = 1;
            }
        }
        Writer(page);
        close(clientfd);
    }

    else{
        Rio_writen(fd, page -> content, strlen(page -> content));
    }
}

void read_requesthdrs(rio_t *rp){
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return ;
}

int parse_uri(char *uri, char *host, char *path, char *port){
    

    char *real_uri = strstr(uri, "//") + 2;
    
    strcpy(port, "80");

    if(real_uri == NULL) return 0;

    char *tmp_path = strstr(real_uri, "/");
    
    char *tmp_port = strstr(real_uri, ":");
    int len = 0;
    if(tmp_port != NULL){
        len = strlen(tmp_port) - strlen(tmp_path);
        tmp_port++;
        strncpy(port, tmp_port, strlen(tmp_port) - strlen(tmp_path));
    }

    if(tmp_path == NULL){
        strncpy(host, real_uri, strlen(real_uri) - len - strlen(tmp_path));
        strcpy(path, "/");
        return 1;
    }

    strcpy(path, tmp_path);
    strncpy(host, real_uri, strlen(real_uri) - strlen(tmp_path) - len);
    
    return 1;
}

void build_request_header(char *host, char *path, char *port, rio_t *rio, char *header){
    static char buf[MAXLINE], host_hdr[MAXLINE], other_hdr[MAXLINE], request_hdr[MAXLINE];
    sprintf(request_hdr, "GET %s HTTP1.0\r\n", path);

    while(Rio_readlineb(rio, buf, MAXLINE) != 0){
        if(strcmp(buf, "\r\n")) break;

        if(strstr(buf, "Host:") == buf){
            strcpy(host_hdr, buf);
            continue;
        }
        if(strstr(buf, "Connection:") != buf && strstr(buf, "User-Agent:") != buf && strstr(buf, "Proxy-Connection") != buf){
            strcat(other_hdr, buf);
            continue;
        }
    }
    if(strlen(host_hdr) == 0)
      sprintf(host_hdr, "Host: %s", host);

    sprintf(header, "%s%s%s%s%s%s\r\n", request_hdr, host_hdr, conn_hdr, proxy_conn_hdr, user_agent_hdr, other_hdr);
    return;
}

void *thread(void *vargp){
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

void init_cache(){
    cache.head = cache.tail = NULL;
    cache.siz = 0;
    Sem_init(&cache.mutex, 0, 1);
    Sem_init(&cache.w, 0, 1);
    cache.read_cnt = 0;
}

web_obj *Reader(char *url){
    P(&cache.mutex);
    cache.read_cnt++;
    if(cache.read_cnt == 1) P(&cache.w);
    V(&cache.mutex);

    web_obj *p;
    web_obj *found = NULL;

    for(p = cache.head; p; p = p -> next){
        if(!strcmp(p -> url, url) || p -> empty){
        //    Writer(p);
            found = p;
            break;
        }
    }

    /*create a new web object*/
    if(!found){
        web_obj *new_obj;
        new_obj = malloc(sizeof(web_obj));
        strcpy(new_obj -> url, url);
        new_obj -> empty = 1;
    //    Writer(new_obj);
        found = new_obj;
    }
    
    P(&cache.mutex);
    cache.read_cnt--;
    if(cache.read_cnt == 0) V(&cache.w);
    V(&cache.mutex);

    return found;
}

void Writer(web_obj *new_obj){
    P(&cache.w);

    cache.siz++;
    
    /*Remove head object*/
    if(cache.siz > 10){
        web_obj *new_head = cache.head -> next;
        free(cache.head);
        cache.head = new_head;
        cache.siz--;
    }

    if(cache.siz == 1){
        cache.head = new_obj;
        cache.tail = new_obj;
    }else{
         cache.tail -> next = new_obj;
        cache.tail = new_obj;
    }

    V(&cache.w);
    return ;
}