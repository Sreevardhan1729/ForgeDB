#include "rdb.h"
#include "datastore.h"
#include <__filesystem/path.h>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <optional>
#include <string_view>
#include <sys/fcntl.h>
#include <unistd.h>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <string>
#include <utility>


void appendEnd(std::string &message){
    message.push_back('\r');
    message.push_back('\n');
}
std::string serializeString(const std::string &data){
    std::string len = std::to_string(data.size());
    std::string message =len;
    appendEnd(message);
    message+=data;
    appendEnd(message);
    return message;
}
std::string serializeSet(std::unordered_set<std::string> &data){
    std::string len = std::to_string(data.size());
    std::string message = len;
    appendEnd(message);
    for(auto d:data){
        message+=serializeString(d);
    }
    return message;
}
std::string serializeList(std::deque<std::string> &data){
    std::string len = std::to_string(data.size());
    std::string message = len;
    appendEnd(message);
    for(auto d:data){
        message+=serializeString(d);
    }
    return message;
}
std::string serializeHash(std::unordered_map<std::string,std::string> &data){
    std::string len = std::to_string(data.size());
    std::string message = len;
    appendEnd(message);
    for(auto &[key,value]:data){
        message+=serializeString(key);
        message+=serializeString(value);
    }
    return message;
    
}
std::string serializeValue(Store &data){
    std::string message="";
    if(auto *it = getTypedData<std::string>(data)){
        message.push_back('S');
        message+=serializeString(*it);
    }
    else if(auto *it = getTypedData<std::unordered_set<std::string>>(data)){
        message.push_back('T');
        message+=serializeSet(*it);
    }
    else if(auto *it = getTypedData<std::deque<std::string>>(data)){
        message.push_back('L');
        message+=serializeList(*it);
    }
    else if(auto *it = getTypedData<std::unordered_map<std::string,std::string>>(data)){
        message.push_back('H');
        message+=serializeHash(*it);
    }
    return message;
}
std::string serializeTtl(std::chrono::steady_clock::time_point &expiresAt){
    std::string message="";
    message.push_back('E');
    auto timeRemaining = expiresAt - std::chrono::steady_clock::now();
    auto absolute_deadline = std::chrono::system_clock::now() + timeRemaining;
    long long seconds = std::chrono::duration_cast<std::chrono::seconds>(absolute_deadline.time_since_epoch()).count();
    message+=std::to_string(seconds);
    appendEnd(message);
    return message;
}
std::string serializeStore(){
    std::string serializeMessage="";
    for(auto [key,value]:store){
        if(value.expiresAt.has_value()){
            serializeMessage+=serializeTtl(value.expiresAt.value());
        }
        serializeMessage+=serializeString(key);
        serializeMessage+=serializeValue(value);
    }
    return serializeMessage;
}

bool checkIfDataEnd(std::string_view message,int &idx){
    if(idx+1 < message.size() && message[idx]=='\r' && message[idx+1]=='\n'){
        idx+=2;
        return true;
    }
    return false;
}
long long getInt(std::string &data,int &pos){
    long long time = 0;
    while(true){
        if(checkIfDataEnd(data, pos)){
            break;
        }
        char c = data[pos++];
        if(!isdigit(c)){
            return -1;
        }
        time = (time*10)+(int)(c-'0');
    }
    return time;
}
std::string deserializeString(std::string &data,int& pos,int currSize){
    std::string value="";
    for(int idx=0;idx<currSize;idx++){
        value.push_back(data[idx+pos]);
    }
    pos+=currSize;
    if(checkIfDataEnd(data, pos)){
        return value;
    }
    return "";
}
std::optional<std::string> getString(std::string &data,int &pos){
    int totalSize = data.size();
    int currSize = getInt(data, pos);
    if(currSize+pos>totalSize){
        store.clear();
        return std::nullopt;
    }
    std::string value = deserializeString(data,pos,currSize);
    if(currSize!=0 && value.empty()){
        store.clear();
        return std::nullopt;
    }
    return value;

}
std::optional<std::deque<std::string>> getList(std::string &data,int &pos){
    int listSize = getInt(data, pos);
    if(listSize==-1) return std::nullopt;
    std::deque<std::string> dqueueValue;
    for(int idx=0;idx<listSize;idx++){
        std::optional<std::string> value = getString(data, pos);
        if(!value.has_value()) return std::nullopt;
        dqueueValue.push_back(value.value());
    }
    return dqueueValue;
}
std::optional<std::unordered_set<std::string>> getSet(std::string &data,int &pos){
    int listSize = getInt(data, pos);
    if(listSize==-1) return std::nullopt;
    std::unordered_set<std::string> setValue;
    for(int idx=0;idx<listSize;idx++){
        std::optional<std::string> value = getString(data, pos);
        if(!value.has_value()) return std::nullopt;
        setValue.insert(value.value());
    }
    return setValue;
}
std::optional<std::unordered_map<std::string,std::string>> getHash(std::string &data,int &pos){
    int listSize = getInt(data, pos);
    if(listSize==-1) return std::nullopt;
    std::unordered_map<std::string,std::string> setHash;
    for(int idx=0;idx<listSize;idx++){
        std::optional<std::string> key = getString(data, pos);
        std::optional<std::string> value = getString(data, pos);
        if(!value.has_value() || !key.has_value()) return std::nullopt;
        setHash[key.value()]=value.value();
    }
    return setHash;
}
std::optional<DataVariant> getValue(std::string &data,int &pos){
    DataVariant dataValue;
    if(data[pos]=='S'){
        pos++;
        std::optional<std::string> value = getString(data, pos);
        if(!value.has_value()){
            return std::nullopt;
        }
        dataValue = value.value();
    }
    else if(data[pos]=='L'){
        pos++;
        std::optional<std::deque<std::string>> value = getList(data, pos);
        if(!value.has_value()){
            return std::nullopt;
        }
        dataValue = value.value();
    }
    else if(data[pos]=='H'){
        pos++;
        std::optional<std::unordered_map<std::string,std::string>> value = getHash(data, pos);
        if(!value.has_value()){
            return std::nullopt;
        }
        dataValue = value.value();
    }
    else if(data[pos]=='T'){
        pos++;
        std::optional<std::unordered_set<std::string>> value = getSet(data, pos);
        if(!value.has_value()){
            return std::nullopt;
        }
        dataValue = value.value();
    }
    return dataValue;
}
int deserializeStore(std::string &data){
    int pos=0;
    int totalSize = data.size();
    while(pos<totalSize){
        std::optional<std::chrono::steady_clock::time_point> expiresAt = std::nullopt;
        if(data[pos]=='E'){
            pos++;
            long long seconds = getInt(data,pos);
            if(seconds==-1){
                store.clear();
                return -1;
            }
            auto expiryTime = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
            auto untilExpiry = expiryTime - std::chrono::system_clock::now();
            expiresAt = std::chrono::steady_clock::now()+untilExpiry;
        }
        std::optional<std::string> key = getString(data, pos);
        if(!key.has_value()){
            return -1;
        }
        std::optional<DataVariant> value = getValue(data, pos);
        if(!value.has_value()){
            return -1;
        }
        if(expiresAt.has_value() && expiresAt<=std::chrono::steady_clock::now()) continue;
        store[key.value()]={value.value(),expiresAt};
    }
    return 1;
}

int saveRDB(const char *filename){
    const char* tempFile = "dump.rdb.tmp";
    int fd = open(tempFile, O_CREAT | O_WRONLY | O_TRUNC,0644);
    if(fd==-1){
        return -1;
    }
    std::string data = serializeStore();
    int length = data.size();
    const char* dataToWrite = data.c_str();
    int bitsWritten=0;
    while(bitsWritten<length){
        int val = write(fd,dataToWrite+bitsWritten,length-bitsWritten);
        if(val==-1){
            unlink(tempFile);
            return -1;
        }
        bitsWritten+=val;
    }
    if(fsync(fd)==-1){
        unlink(tempFile);
        return -1;
    }
    close(fd);
    rename(tempFile,filename);
    return 1;
}
int loadRDB(const char *filename){
    int fd = open(filename, O_RDONLY,0644);
    if(fd==-1){
        return 0;
    }std::string data;
    while(true){
        char buffer[1024];
        int numberOfBytesRead = read(fd, buffer, 1024);
        if(numberOfBytesRead==0 || numberOfBytesRead==-1){
            break;
        }
        data.append(buffer,numberOfBytesRead);
    }
    if(deserializeStore(data)==-1){
        return -1;
    }
    
    close(fd);
    return 1;
}