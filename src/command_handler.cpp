#include "command_handler.h"
#include "pubsub.h"
#include "resp_message.h"
#include <algorithm>
#include<functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include "aof.h"
#include "resp_serializer.h"

static std::unordered_set<std::string> writeCommands =
  {"SET","DEL","EXPIRE","LPUSH","RPUSH","LPOP","RPOP","HSET","HDEL","SADD","SREM"};
static std::unordered_map<std::string, std::function<RespMessage(const RespMessage&)>> dispatchTable={
    {"PING",handlePing},
    {"GET",handleGet},
    {"SET",handleSet},
    {"DEL",handleDel},
    {"TTL",handleTtl},
    {"EXPIRE",handleExpire},
    {"LPUSH",handleLpush},
    {"RPUSH",handleRpush},
    {"LPOP",handleLpop},
    {"RPOP",handleRpop},
    {"LRANGE",handleLrange},
    {"HSET",handleHset},
    {"HGET",handleHget},
    {"HDEL",handleHdel},
    {"HGETALL",handleHgetall},
    {"SADD",handleSadd},
    {"SMEMBERS",handleSmembers},
    {"SISMEMBER",handleSismember},
    {"SREM",handleSrem},
    {"SINTER",handleSinter},
};
static std::unordered_map<std::string, std::function<RespMessage(const RespMessage&,int)>> pubsubDistatchTable={
    {"PUBLISH",handlePublish},
    {"SUBSCRIBE",handleSubscribe},
    {"UNSUBSCRIBE",handleUnSubscribe},
};
RespMessage handleCommand(const RespMessage &message,int clientFd){
    std::string commandName = message.elements[0].data;
    std::transform(commandName.begin(),commandName.end(),commandName.begin(),::toupper);
    auto pit = pubsubDistatchTable.find(commandName);
    if(pit!=pubsubDistatchTable.end()){
        return pit->second(message,clientFd);
    }   
    auto it = dispatchTable.find(commandName);
    if(it!=dispatchTable.end()){
        RespMessage result = it->second(message);
        if(result.type!=RespType::Error && result.type!=RespType::Null){ 
            if(writeCommands.find(commandName)!=writeCommands.end()){
                writeToAof(resp_serializer(message));
            }
        }
        return result;
    }
    else{
        return errorMessage("Invlaid Dispatcher command");
    }
}