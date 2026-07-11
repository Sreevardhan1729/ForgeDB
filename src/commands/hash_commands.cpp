#include "hash_commands.h"
#include "../datastore.h"
#include "../utils.h"

RespMessage handleHset(const RespMessage &message){
    if(message.elements.size()<4 || (message.elements.size()&1)){
        return errorMessage("Invalid number of argument given");
    }
    auto it = store.find(message.elements[1].data);
    int count=0;
    if(it!=store.end()){
        auto *value = getTypedData<std::unordered_map<std::string, std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::unordered_map<std::string, std::string> &currHash = *value;
        for(int idx = 2;idx<message.elements.size();idx+=2){
            std::string fieldKey = message.elements[idx].data;
            std::string fieldValue = message.elements[idx+1].data;
            if(currHash.find(fieldKey)==currHash.end()){
                count++;
            }
            currHash[fieldKey]=fieldValue;
        }
    }
    else{
        std::unordered_map<std::string, std::string> currHash;
        for(int idx = 2;idx<message.elements.size();idx+=2){
            std::string fieldKey = message.elements[idx].data;
            std::string fieldValue = message.elements[idx+1].data;
            currHash[fieldKey]=fieldValue;
            count++;
        }
        store[message.elements[1].data] = {currHash};
    }
    return RespMessage{RespType::Integer,std::to_string(count)};
}
RespMessage handleHget(const RespMessage &message){
    if(message.elements.size()!=3){
        return errorMessage("HGET need to have exactly 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_map<std::string, std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        auto fieldIt = value->find(message.elements[2].data);
        if(fieldIt!=value->end()){
            return RespMessage{RespType::Bulk,fieldIt->second};
        }
        return RespMessage{RespType::Null};
    }
    return RespMessage{RespType::Null};
}
RespMessage handleHdel(const RespMessage &message){
    if(message.elements.size()<3){
        return errorMessage("DEL need to have atleast 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    int count=0;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_map<std::string, std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        for(int idx = 2;idx<message.elements.size();idx++){
            auto fieldIt = value->find(message.elements[idx].data);
            if(fieldIt!=value->end()){
                count++;
                value->erase(fieldIt);
            }
        }
        if(value->size()==0){
            store.erase(it);
        }
    }
    return RespMessage{RespType::Integer,std::to_string(count)};
}
RespMessage handleHgetall(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("GETALL need to have exactly 1 arguments");
    }
    auto it = store.find(message.elements[1].data);
    RespMessage result;
    result.type=RespType::Array;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_map<std::string, std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        for(auto mapIt : *value){
            result.elements.push_back(RespMessage{RespType::Bulk,mapIt.first});
            result.elements.push_back(RespMessage{RespType::Bulk,mapIt.second});
        }
    }
    return result;
}