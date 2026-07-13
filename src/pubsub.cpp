#include "pubsub.h"
#include "resp_message.h"
#include "resp_serializer.h"
#include "utils.h"
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
static std::unordered_map<std::string, std::unordered_set<int>> subscribers;
static std::unordered_map<int, std::unordered_set<std::string>> channels;
RespMessage getMessage(std::string message,std::string &channel,int cnt){
    RespMessage result;
    result.type = RespType::Array;
    RespMessage messageToWrite = RespMessage{RespType::Bulk,message};
    RespMessage channelMessage = RespMessage{RespType::Bulk,channel};
    RespMessage clientCnt = RespMessage{RespType::Integer,std::to_string(cnt)};
    result.elements = {messageToWrite,channelMessage,clientCnt};
    return result;
}
RespMessage getPublishMessage(std::string message,std::string channel,std::string content){
    RespMessage result;
    result.type = RespType::Array;
    RespMessage messageToWrite = RespMessage{RespType::Bulk,message};
    RespMessage channelMessage = RespMessage{RespType::Bulk,channel};
    RespMessage contentToWrite = RespMessage{RespType::Bulk,content};
    result.elements = {messageToWrite,channelMessage,contentToWrite};
    return result;
}

RespMessage handleSubscribe(const RespMessage &message, int clientFd){
    if(message.elements.size()<2){
        return errorMessage("SUBSCRIBE need at least 1 argument");
    }
    for(int idx=1;idx<message.elements.size();idx++){
        std::string channel = message.elements[idx].data;
        subscribers[channel].insert(clientFd);
        channels[clientFd].insert(channel);
        RespMessage curr = getMessage("subscribe",channel,channels[clientFd].size());
        std::string dataToWrite = resp_serializer(curr);
        write(clientFd, dataToWrite.c_str(), dataToWrite.size());
    }
    return RespMessage{RespType::Null};
};

RespMessage handleUnSubscribe(const RespMessage &message, int clientFd){
    if(message.elements.size()<2){
        return errorMessage("UNSUBSCRIBE need at least 1 argument");
    }
    if(channels.find(clientFd)==channels.end()){
        return RespMessage{RespType::Null};
    }
    auto& it = channels[clientFd];
    for(int idx=1;idx<message.elements.size();idx++){
        std::string channel = message.elements[idx].data;
        if(it.find(channel)!=it.end()){
            it.erase(channel);
            if(subscribers.find(channel)!=subscribers.end()){
                auto& subIt = subscribers[channel];
                subIt.erase(clientFd);
                if(subIt.size()==0){
                    subscribers.erase(channel);
                }
            }
        }
        RespMessage curr = getMessage("unsubscribe",channel,it.size());
        std::string dataToWrite = resp_serializer(curr);
        write(clientFd, dataToWrite.c_str(), dataToWrite.size());
    }
    if(it.size()==0){
        channels.erase(clientFd);
    }
    return RespMessage{RespType::Null};
};

RespMessage handlePublish(const RespMessage &message, int clientFd){
     if(message.elements.size()!=3){
        return errorMessage("PUBLISH need 2 arguments");
    }
    auto it = subscribers.find(message.elements[1].data);
    if(it==subscribers.end()){
        return RespMessage{RespType::Integer,"0"};
    }
    int cnt=0;
    for(auto iter = it->second.begin(); iter != it->second.end();){
        int fd = *iter;
        RespMessage curr = getPublishMessage("message", message.elements[1].data, message.elements[2].data);
        std::string dataToWrite = resp_serializer(curr);
        write(fd, dataToWrite.c_str(), dataToWrite.size());
        cnt++;
        ++iter;
    }
    return RespMessage{RespType::Integer,std::to_string(cnt)};   
}
RespMessage unsubscribeClient(int clientFd){
    auto it = channels.find(clientFd);
    if(it==channels.end()){
        return RespMessage{RespType::Null};
    }
    for(auto iter = it->second.begin();iter!=it->second.end();){
        std::string channel = *iter;
        if(subscribers.find(channel)!=subscribers.end()){
            subscribers[channel].erase(clientFd);
        }
        ++iter;
    }
    channels.erase(it);
    return RespMessage{RespType::Null};
}