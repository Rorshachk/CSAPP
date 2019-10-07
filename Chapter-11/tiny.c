#include <csapp.h>

void doit(int fd);
void clienterror(int fd, char *filename, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *filename, char *uri, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void read_requesthdrs(rio_t *rp);
void get_filetype(char *filename, char *filetype);

int main(int argc, char const *argv[])
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    /*
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }*/
    port = 80;

    listenfd = Open_listenfd(port);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("server connected to (%s:%s)\n", client_hostname, client_port);
        doit(connfd);
        Close(connfd);
    }
    return 0;
}

void doit(int fd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], filename[MAXLINE], cgirgs[MAXLINE];
    rio_t rio;
    int is_static;
    struct stat sbuf;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    printf("request header:\n%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcmp(method, "GET")){
        clienterror(fd, method, "501", "Not implemented", "Tiny doesn't implement this method");
        return;
    }
    read_requesthdrs(&rio);
    
    is_static = parse_uri(uri, filename, cgirgs);

    if(stat(filename, &sbuf) < 0){
        clienterror(fd, filename, "404", "Not found", "Tiny can'n find this file");
        return ;
    }
    
    if(is_static){
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Not found", "Tiny can't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else{
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Forbidden", "Tiny can't run the CGI program");
            return ;
        }
        serve_dynamic(fd, filename, cgirgs);
    }
}

int parse_uri(char *uri, char *filename, char *cgiargs){
    char *ptr;   //如果把动态文件当成静态处理的话，就会自动下载

    if(!strstr(uri, "cgi-bin")){
        memset(cgiargs, 0, sizeof(cgiargs));
        strcpy(filename, ".");
        strcat(filename, uri);
        if(uri[strlen(uri) - 1] == '/') strcat(filename, "index.html");
        return 1;
    }else{
        ptr = index(uri, '?');
        if(ptr){
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }else  strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
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

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void serve_static(int fd, char *filename, int filesize) 
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    /* Send response headers to client */
    get_filetype(filename, filetype);       //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    //line:netp:servestatic:beginserve
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));       //line:netp:servestatic:endserve

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);    //line:netp:servestatic:open
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
    Close(srcfd);                           //line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize);         //line:netp:servestatic:write
    Munmap(srcp, filesize);                 //line:netp:servestatic:munmap
}

void get_filetype(char *filename, char *filetype){
    if(strstr(filename, ".html")) strcpy(filetype, "text/html");
    else if(strstr(filename, "jpg")) strcpy(filetype, "image/jpg");
    else if(strstr(filename, "png")) strcpy(filetype, "image/png");
    else if(strstr(filename, "gif")) strcpy(filetype, "image/gif");
    else if(strstr(filename, "jpeg")) strcpy(filetype, "image/jpeg");
    else strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
    char buf[MAXLINE], *emptylist[] = {NULL};
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if(Fork() == 0){
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    
    wait(NULL);
}