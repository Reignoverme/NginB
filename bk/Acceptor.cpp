#include "Acceptor.h"

Acceptor::Acceptor(EventLoop* loop,
                   int port,
                   boost::asio::ip::address localaddr)
    : loop_(loop),
      sockfd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(port),
      localaddr_(localaddr),
      acceptChannel_(new Channel(sockfd_, loop))
{
    if(sockfd_ < 0)
    {
        std::cout << "create socket failed. error:"
            << std::endl;
        exit(-1);
    }

    int flags = fcntl(sockfd_, F_GETFL);
    fcntl(sockfd_, F_SETFL, flags|O_NONBLOCK);

    int yes=1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    acceptChannel_->setReadCallback(
        boost::bind(&Acceptor::handleRead, this));
}

void Acceptor::handleRead()
{
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    int connfd = accept4(sockfd_, (struct sockaddr*)&peer,
                         &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
    std::cout << "connection accepted from "
        << inet_ntoa(peer.sin_addr) << ": " << ntohs(peer.sin_port)
        << " connfd: " << connfd <<std::endl;

    if(connfd < 0)
    {
        if(errno & (EAGAIN|EWOULDBLOCK))
        {
            std::cout << "accept() not ready.";
            return;
        }
        
        if(errno == ECONNABORTED)
        {
            std::cout << "peer closed prematurely.";
        }
        
        // TODO other errors
        return;
    }

    int pPort = peer.sin_port;
    boost::asio::ip::address pAddr = 
        boost::asio::ip::address::from_string(inet_ntoa(peer.sin_addr));

    // use connections_ to manage connection life time.
    boost::shared_ptr<Connection> c(new Connection(loop_, this, connfd, pPort, pAddr));
    c->getRequest();
    c->connEstablished();
    connections_[c->fd()] = c;
}

void Acceptor::listen()
{
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port_);
    local.sin_addr.s_addr = htonl(localaddr_.to_v4().to_ulong());

    std::cout<<"listen on " << ntohs(local.sin_port)<< std::endl;

    if(::bind(sockfd_, (struct sockaddr*)&local, sizeof local) == -1)
    {
        std::cout << "bind failed. error: " << strerror(errno);
        exit(-1);
    }
    if(::listen(sockfd_, 10) == -1)    //backlog doesn't matter
    {
        std::cout << "listen failed. error: " << strerror(errno);
        exit(-1);
    }

    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if(lock.owns_lock())
    {
        if(!acceptChannel_->isRactive())
        {
            acceptChannel_->enableReading();
        }
    } else {
        if(acceptChannel_->isRactive()) 
        {
            acceptChannel_->disableEvent();
        }
    }// if-else
}

