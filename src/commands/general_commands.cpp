#include "general_commands.h"
#include "../utils.h"
#include "../datastore.h"
RespMessage handlePing(const RespMessage &message){
    if(message.elements.size()==1){
        return RespMessage{RespType::Simple,"PONG"};
    }
    else if(message.elements.size()==2){
        return RespMessage{RespType::Bulk,message.elements[1].data};
    }
    else{
        return errorMessage("PING accepts only 0 or 1 argument");
    }
}

RespMessage handleDel(const RespMessage &message){
     if(message.elements.size()<2){
        return errorMessage("DEL should have atleast 1 argument");
    }
    int count=0;
    int numElements = message.elements.size();
    for(int idx=1;idx<numElements;idx++){
        auto it = store.find(message.elements[idx].data);
        if(it!=store.end()){
            store.erase(it);
            count++;
        }
    }
    return RespMessage{RespType::Integer,std::to_string(count)};
}

RespMessage handleTtl(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("TTL should have atleast 1 argument");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        if(it->second.expiresAt.has_value()){
            auto timeRemanining = *it->second.expiresAt - std::chrono::steady_clock::now();
            int seconds = std::chrono::duration_cast<std::chrono::seconds>(timeRemanining).count();
            if(seconds<=0){
                store.erase(it);
                return RespMessage{RespType::Integer,"-2"};
            }
            else{
                return RespMessage{RespType::Integer,std::to_string(seconds)};
            }
        }
        else{
            return RespMessage{RespType::Integer,"-1"};
        }
    }
    return RespMessage{RespType::Integer,"-2"};
}

RespMessage handleExpire(const RespMessage &message){
    if(message.elements.size()!=3){
        return errorMessage("EXPIRE should have atleast 1 argument");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        std::optional<int> val = getNumber(message.elements[2].data);
        if(!val.has_value()){
            return errorMessage("Expiratoion number format wrong");
        }
        std::chrono::steady_clock::time_point expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(val.value());
        it->second.expiresAt = expiresAt;
        return RespMessage{RespType::Integer,"1"};
    }
    return RespMessage{RespType::Integer,"0"};
}