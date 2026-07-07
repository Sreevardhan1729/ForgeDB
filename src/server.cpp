#include"command_handler.h"
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <ctime>
#include<fcntl.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include "resp_message.h"
#include"resp_parser.h"
#include "resp_serializer.h"

int main(){
    static std::unordered_map<int, std::string> per_client_buffer;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(socket_fd, F_SETFL,O_NONBLOCK);
    sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    int val = bind(socket_fd, (const struct sockaddr *)&address, sizeof(address));
    if(val==-1){
        return -1;
    }
    listen(socket_fd, 16);

    int kq = kqueue();
    if(kq==-1){
        return -1;
    }

    struct kevent events;
    EV_SET(&events, socket_fd, EVFILT_READ, EV_ADD, 0, 0, 0);

    if(kevent(kq,&events,1,nullptr,0,nullptr)==-1){
        return -1;
    }

    std::vector<struct kevent> eventList(64);
    auto lastCleanUp = std::chrono::steady_clock::now();
    struct timespec timout = {1,0};
    while(true){
        int numEvents = kevent(kq, nullptr, 0, eventList.data(), 64, &timout);
        if(numEvents==-1){
            break;
        }
        auto now = std::chrono::steady_clock::now();
        if(now - lastCleanUp >= std::chrono::seconds(1)){
            cleanUpExpiredKeys();
            lastCleanUp = now;
        }

        for(int idx=0;idx<numEvents;idx++){
            struct kevent event = eventList[idx];
            if(event.filter == EVFILT_READ){
                if(event.ident == socket_fd){
                    int clientFd = accept(socket_fd, NULL, NULL);
                    if(clientFd==-1){
                        continue;
                    }
                    fcntl(clientFd, F_SETFL,O_NONBLOCK);
                    struct kevent clientEvents;
                    EV_SET(&clientEvents,clientFd,EVFILT_READ,EV_ADD,0,0,0);
                    kevent(kq,&clientEvents,1,nullptr,0,nullptr);
                }
                else{
                    char buffer[1024];
                    int numberOfBytesRead = read(event.ident, buffer, sizeof(buffer));
                    if(numberOfBytesRead==0){
                        per_client_buffer.erase(event.ident);
                        close(event.ident);
                        continue;
                    }
                    else if(numberOfBytesRead==-1){
                        continue;
                    }
                    else{
                        per_client_buffer[event.ident].append(buffer,numberOfBytesRead);
                        std::string &currData = per_client_buffer[event.ident];
                        while(true){
                            int pos = 0;
                            std::optional<RespMessage> dataRead = resp_parser(currData, pos);
                            RespMessage data;
                            if(!dataRead.has_value()){
                                break;
                            }
                            else if(dataRead.value().type==RespType::Array){
                                data = handleCommand(dataRead.value());
                                currData.erase(0,pos);
                            }
                            else{
                                data = RespMessage{RespType::Error,"Data Format Incorrect"};
                                currData.clear();
                            }
                            std::string dataWrite = resp_serializer(data);
                            write(event.ident, dataWrite.c_str(), dataWrite.size());
                            if(dataRead.value().type!=RespType::Array){
                                break;
                            }
                        }
                        if(currData.empty()){
                            per_client_buffer.erase(event.ident);
                        }
                    }
                }
            }
        }
    }
}