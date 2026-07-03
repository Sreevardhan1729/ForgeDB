#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/event.h>
#include<fcntl.h>
#include <unistd.h>
#include <vector>
int main(){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(socket_fd, F_SETFL,O_NONBLOCK);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    if(bind(socket_fd, (const sockaddr *)&address,sizeof(address))==-1){
        return -1;
    }
    listen(socket_fd, 5);

    int kq = kqueue();
    if(kq==-1){
        return -1;
    }
    struct kevent events;
    EV_SET(&events,socket_fd,EVFILT_READ,EV_ADD,0,0,NULL);

    if(kevent(kq, &events, 1, nullptr, 0, nullptr)==-1){
        return -1;
    }
    std::vector<struct kevent> eventList(64);

    while(true){
        int numEvents = kevent(kq, NULL, 0, eventList.data(), 64, nullptr);
        if(numEvents==-1){
            break;
        }
        for(int idx=0;idx<numEvents;idx++){
            struct kevent currentEvent = eventList[idx];
            int currentFd = currentEvent.ident;

            if(currentEvent.filter == EVFILT_READ){
                if(currentFd==socket_fd){
                    int clientFd = accept(socket_fd,NULL,NULL);

                    if(clientFd!=-1){
                        fcntl(clientFd, F_SETFL,O_NONBLOCK);
                        struct kevent clientEvents;
                        EV_SET(&clientEvents,clientFd,EVFILT_READ,EV_ADD,0,0,NULL);
                        kevent(kq, &clientEvents, 1, nullptr, 0, nullptr);
                    }
                }
                else{
                    char buffer[100];
                    int numberOfBytesRead = read(currentFd, buffer, 99);
                    if(numberOfBytesRead==0){
                        close(currentFd);
                    }
                    else if(numberOfBytesRead==-1){
                        continue;
                    }
                    else{
                        write(currentFd, buffer, numberOfBytesRead);
                    }
                }
            }
        }
    }
}