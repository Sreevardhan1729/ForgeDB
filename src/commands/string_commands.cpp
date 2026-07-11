#include "string_commands.h"
#include "../datastore.h"
#include "../utils.h"
RespMessage handleSet(const RespMessage &message){
    if(message.elements.size()!=3 && (message.elements.size()!=5 || message.elements[3].data!="EX")){
        return errorMessage("SET accepts only 2 or 4 arguments");
    }
    if(message.elements.size()==5){
        std::optional<int> val = getNumber(message.elements[4].data);
        if(!val.has_value()){
            return errorMessage("Expiratoion value format wrong");
        }
        if(val.value()>=0){
            std::chrono::steady_clock::time_point expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(val.value());
            store[message.elements[1].data] = Store{message.elements[2].data,expiresAt};
        }
        else{
            return errorMessage("Invalid expiration value");
        }
    }
    else{
        store[message.elements[1].data] = Store{message.elements[2].data,std::nullopt};
    }
    return RespMessage{RespType::Simple,"OK"};
}

RespMessage handleGet(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("GET accepts only 1 argument");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        if(it->second.expiresAt.has_value() && *it->second.expiresAt <= std::chrono::steady_clock::now()){
            store.erase(it);
            return RespMessage{RespType::Null};
        }
        auto* value = getTypedData<std::string>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        return RespMessage{RespType::Bulk,*value};
            
    }
    return RespMessage{RespType::Null};
}