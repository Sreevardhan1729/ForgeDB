#include "command_handler.h"
#include "resp_message.h"
#include <algorithm>
#include<functional>
#include <string>
#include <unordered_map>

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

RespMessage handleCommand(const RespMessage &message){
    std::string commandName = message.elements[0].data;
    std::transform(commandName.begin(),commandName.end(),commandName.begin(),::toupper);
    auto it = dispatchTable.find(commandName);
    if(it!=dispatchTable.end()){
        return it->second(message);
    }
    else{
        return errorMessage("Invlaid Dispatcher command");
    }
}