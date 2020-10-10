#include "TcpServer.h"

TcpServer::TcpServer(){

}

TcpServer::~TcpServer(){

}

int TcpServer::createAndListen(){
    int on = 1;
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    fcntl(listenfd,F_SETFL,O_NONBLOCK); //no-block io
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if(-1 == bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr))){
        cout << "bind error,errno:" << errno << endl;
    }

    if(-1 == listen(listenfd,MAX_LISTENFD)){
        cout << "listen error,errno:" << errno << endl;
    }

    return listenfd;
}

void TcpServer::start(){
    struct epoll_event ev,events[MAX_EVENTS];
    int listenfd,connfd,sockfd;
    int readlength;
    char line[MAX_LINE];
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(struct sockaddr_in);

    //int epoll_create(int size)    
    //创建一个epoll文件描述符
    int epollfd = epoll_create(1);

    if(epollfd < 0)
        cout << "epoll_create error,error:" << epollfd << endl;
    listenfd = createAndListen();
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;

    //int epoll_ctl(int epfd,int op,int fd,struct epoll_event *event)
    //将socket描述符加入/移出epoll监听，修改注册事件
    epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&ev);

    for(;;){
        //int epoll_wait(int epfd,strcut epoll_event *events,int maxevents,int timeout)
        //在epoll描述符上等待事件的发生，并获得事件
        int fds = epoll_wait(epollfd,events,MAX_EVENTS,-1);
        if(fds == -1){
            cout << "epoll_wait error,errno:" << errno << endl;
            break;
        }
        for(int i = 0;i < fds;i++){
            if(events[i].data.fd == listenfd){
                connfd = accept(listenfd,(sockaddr *)&cliaddr,(socklen_t *)&clilen);
                if(connfd > 0){
                    cout << "new connection from"
                         << " " << inet_ntoa(cliaddr.sin_addr)
                         << ":" << ntohs(cliaddr.sin_port) << "]"
                         << " accept socket fd:" << connfd
                         << endl;
                }
                else{
                    cout << "accep error,connd:" << connfd
                         << " errno:" << errno << endl;
                }
                fcntl(connfd,F_SETFL,O_NONBLOCK);
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                if(-1 == epoll_ctl(epollfd,EPOLL_CTL_ADD,connfd,&ev))
                    cout << "epoll_ctrl error,errno:" << errno << endl;
            }
            else if(events[i].events & EPOLLIN){
                if((sockfd = events[i].data.fd) < 0){
                    cout << "EPOLLIN sockfd < 0 error " << endl;
                    continue;
                }
                bzero(line,MAX_LINE);
                if((readlength = read(sockfd,line,MAX_LINE)) < 0){
                    if(errno == ECONNRESET){
                        cout << "ECONNRESET closed socket fd:" << events[i].data.fd << endl;
                        close(sockfd);
                    }
                }
                else if(readlength == 0){
                    cout << "read 0 closed socket fd:" << events[i].data.fd << endl;
                    close(sockfd);
                }
                else{
                    if(write(sockfd,line,readlength) != readlength)
                        cout << "errno: not finished one time" << endl;
                }
            }
        }
    }
}