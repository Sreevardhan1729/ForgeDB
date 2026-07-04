#include "command_handler.h"
#include "resp_message.h"
#include <functional>
#include <string>
#include <unordered_map>


static std::unordered_map<std::string, std::string> store;

RespMessage handlePing(const RespMessage &message){
    if(message.elements.size()==1){
        return RespMessage{RespType::Simple,"PONG"};
    }
    else if(message.elements.size()==2){
        return RespMessage{RespType::Bulk,message.elements[1].data};
    }
    else{
        return RespMessage{RespType::Error,"PING accepts only 0 or 1 Argument"};
    }

}
RespMessage handleSet(const RespMessage &message){
    if(message.elements.size()!=3){
        return RespMessage{RespType::Error,"SET accepts only 2 Arguments"};
    }
    store[message.elements[1].data] = message.elements[2].data;
    return RespMessage{RespType::Simple,"OK"};
}
RespMessage handleGet(const RespMessage &message){
    if(message.elements.size()!=2){
        return RespMessage{RespType::Error,"GET accepts only 1 Argument"};
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        return RespMessage{RespType::Bulk,it->second};
    }
    return RespMessage{RespType::Null};
}

static std::unordered_map<std::string, std::function<RespMessage(const RespMessage&)>> dispatchTable={
    {"PING",handlePing},
    {"GET",handleGet},
    {"SET",handleSet},
};

RespMessage handleCommand(const RespMessage &message){
    if(message.type!=RespType::Array){
        return RespMessage{RespType::Error,"Data Format Incorrect"};
    }
    std::string commandName = message.elements[0].data;
    auto it = dispatchTable.find(commandName);
    if(it!=dispatchTable.end()){
        return it->second(message);
    }
    else{
        return RespMessage{RespType::Error,"Invlaid Dispater command"};
    }
}