#include <cerrno>
#include <cstddef>
#include<fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main(){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(socket_fd, F_SETFL,O_NONBLOCK);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    int val = bind(socket_fd, (const sockaddr *)&address, sizeof(address));

    if(val==-1){
        return -1;
    }

    listen(socket_fd, 10);
    std::vector<int> client_fds;
    auto closeConnection = [](std::vector<int> & client_fds, int& idx,int &numberOfClients){
        int fd = client_fds[idx];
        close(fd);
        std::swap(client_fds[idx],client_fds[numberOfClients-1]);
        client_fds.pop_back();
        idx--;
        numberOfClients--;

    };
    while(true){
        int client_fd = accept(socket_fd, NULL, NULL);
        if(client_fd!=-1){
            fcntl(client_fd, F_SETFL,O_NONBLOCK);
            client_fds.push_back(client_fd);
        }
        int numberOfClients = client_fds.size();
        for(int idx=0;idx<numberOfClients;idx++){
            int fd = client_fds[idx];
            char buffer[100];
            int numberOfBytesRead = read(fd, buffer, 99);
            if(numberOfBytesRead==0){
                closeConnection(client_fds, idx, numberOfClients);
                continue;
            }
            else if(numberOfBytesRead ==-1){
                if(errno==EAGAIN){
                    continue;
                }
                else{
                    closeConnection(client_fds, idx, numberOfClients);
                }
            }
            else{
                write(fd, buffer, numberOfBytesRead);
            }
        }
    }
    return 0;
}