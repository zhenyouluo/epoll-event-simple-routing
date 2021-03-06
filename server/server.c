
#include<stdio.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>

#include "debug.h"
#include "event_manager.h"
#include "request.h"

#define BUFFSIZE 1024

void read_cb(int socket_fd,void **data)
{
    // NOTE -> read is also invoked on accept and connect
    INFO("in read_cb\n");
    // we just read data and print
    char buf[BUFFSIZE]={0};
    int val = read(socket_fd, buf, BUFFSIZE);
    
    if (val>0)
    {
        int dataLen = request_data_parse(buf);
        if(dataLen > 0){
            
            request_save_srcfd_to_hash(socket_fd,data);
            int dest_fd = request_get_destfd_from_hash();
            
            if (dest_fd != -1) {
                
                int sent=write(dest_fd,buf,dataLen);
                
                if(sent==-1){
                    INFO("sent error\n");
                }
                else{
                    LOG("sent:%d\n",sent);
                }
            }
        }
    }

}


void close_cb(int socket_fd,void *data)
{
    INFO("in close_cb\n");
    
    request_remove_fd(data);
    // close the socket, we are done with it
    event_manager_remove_element(socket_fd);    
}

void accept_cb(int socket_fd)
{
    INFO("in accept_cb\n");
    
    // accept the connection 
    struct sockaddr_in clt_addr;
    socklen_t clt_len = sizeof(clt_addr);
    int listenfd = accept(socket_fd, (struct sockaddr*) &clt_addr, &clt_len);
    fcntl(listenfd, F_SETFL, O_NONBLOCK);
    //fprintf(stderr, "got the socket %d\n", listenfd);
    
    // set flags to check 
    uint32_t flags = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
    
    // add file descriptor to poll event
    event_manager_add_element( listenfd, flags, 0);
    
}

/*
void connect_cb(int socket_fd)
{
}

void write_cb(int socket_fd)
{
}
*/

int main()
{
    //如果尝试send到一个disconnected socket上，就会让底层抛出一个SIGPIPE信号。这里防止进程退出
    //http://blog.sina.com.cn/s/blog_502d765f0100kopn.html
    //SIGPIPE handle by kernel	
    struct sigaction sa;
    sa.sa_handler=SIG_IGN;
    sigaction(SIGPIPE,&sa,0);

    // create a TCP socket, bind and listen
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in svr_addr;
    memset(&svr_addr, 0 , sizeof(svr_addr));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = htons(INADDR_ANY); //自动获得本机IP
    svr_addr.sin_port = htons(5372);  //端口设置为8080

    bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
    listen(sock, 10);

    //设置为非阻塞 http://www.cnblogs.com/andtt/articles/2178875.html
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // create a poll event object, with time out of 1 sec
    event_manager_init(1000);
    
    // add sock to poll event
    event_manager_add_element(sock, EPOLLIN, ACCEPT_CB);
 
    // start the event loop
    while(1){
        event_manager_process();
    }

    return 0;
}

