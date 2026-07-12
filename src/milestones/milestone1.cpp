#include<fcntl.h>
#include <sys/_types/_s_ifmt.h>
#include <sys/fcntl.h>
#include<unistd.h>
int main(){
    int fd = open("test.txt", O_RDWR | O_CREAT,0644);
    if(fd==-1){
        const char* errorMessageForOpen = "File could not be opened";
        write(2,errorMessageForOpen,25);
        return -1;
    }
    const char* dataToWrite = "hello file descriptors";
    write(fd, dataToWrite, 22);
    close(fd);

    int fd1 = open("test.txt", O_RDWR,0644);

    char buffer[100];
    int numberOfBytesRead = read(fd1, buffer, 99);

    close(fd1);
    close(1);
    int fd2 = open("test1.txt",O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR,0666);
    write(1,buffer,numberOfBytesRead);
    close(fd1);
}