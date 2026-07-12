#include "aof.h"
#include "command_handler.h"
#include "resp_message.h"
#include "resp_parser.h"
#include <cstdio>
#include <sys/fcntl.h>
#include <unistd.h>

static int fd;
static bool replaying = false;
void initAof(const char* filename){
    fd = open(filename,O_RDWR | O_APPEND | O_CREAT,0644);
}
void fsyncAof(){
    fsync(fd);
}
void replayAof(){
    std::string data;
    lseek(fd, 0, SEEK_SET);
    replaying=true;
    while(true){
        char buffer[1024];
        int numberofBitsRead = read(fd, buffer, 1024);
        if(numberofBitsRead==0 || numberofBitsRead==-1){
            break;
        }
        data.append(buffer,numberofBitsRead);
        while(true){
            int pos=0;
            std::optional<RespMessage> result = resp_parser(data, pos);
            if(!result.has_value()){
                break;
            }
            if(result.value().type==RespType::Array){
                handleCommand(result.value());
                data.erase(0,pos);
            }
            else{
                data.clear();
                break;
            }
        }
    }
    replaying=false;
}
void writeToAof(std::string data){
    if(fd==-1 || replaying){
        return;
    }
    const char* dataToAppend = data.c_str();
    int bytesWritten = 0;
    int totalLength = data.size();
    while(bytesWritten<totalLength){
        int val = write(fd, dataToAppend+bytesWritten, totalLength-bytesWritten);
        if(val==-1 || val==0){
            break;
        }
        bytesWritten+=val;
    }
}