#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
int main(){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    int val = bind(socket_fd,(struct sockaddr *)&address, sizeof(address));
    if(val==-1){
        return -1;
    }

    listen(socket_fd, 1);

    int client_fd = accept(socket_fd, NULL, NULL);
    while(true){
        char buffer[100];
        int numberOfBytesRead = read(client_fd,buffer,99);
        if(numberOfBytesRead==0){
            break;
        }
        write(client_fd,buffer,numberOfBytesRead);
    }
    close(client_fd);
    close(socket_fd);
}