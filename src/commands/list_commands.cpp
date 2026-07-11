#include "list_commands.h"
#include "../datastore.h"
#include "../utils.h"

RespMessage handleLpush(const RespMessage &message){
    if(message.elements.size()<3){
        return errorMessage("LPUSH should have atleast 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    
    if(it!=store.end()){
        auto *value = getTypedData<std::deque<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = *value;
        for(int idx=2;idx<message.elements.size();idx++){
            currList.push_front(message.elements[idx].data);
        }
        return RespMessage{RespType::Integer,std::to_string(currList.size())};
    }
    else{
        std::deque<std::string> currList;
        for(int idx=2;idx<message.elements.size();idx++){
            currList.push_front(message.elements[idx].data);
        }
        store[message.elements[1].data] = {currList};
        return RespMessage{RespType::Integer,std::to_string(currList.size())};
    }
}
RespMessage handleRpush(const RespMessage &message){
    if(message.elements.size()<3){
        return errorMessage("LPUSH should have atleast 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        auto *value = getTypedData<std::deque<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string>& currList = *value;
        for(int idx=2;idx<message.elements.size();idx++){
            currList.push_back(message.elements[idx].data);
        }
        return RespMessage{RespType::Integer,std::to_string(currList.size())};
    }
    else{
        std::deque<std::string> currList;
        for(int idx=2;idx<message.elements.size();idx++){
            currList.push_back(message.elements[idx].data);
        }
        store[message.elements[1].data] = {currList};
        return RespMessage{RespType::Integer,std::to_string(currList.size())};
    }
}
RespMessage handleLpop(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("LPOP should have 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        auto *value = getTypedData<std::deque<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = *value;
        if(currList.empty()){
            return RespMessage{RespType::Null};
        }
        std::string front_element = currList.front();
        currList.pop_front();
        if(currList.empty()){
            store.erase(it);
        }
        return RespMessage{RespType::Bulk,front_element};
    }
    return RespMessage{RespType::Null};
}
RespMessage handleRpop(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("RPOP should have 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        auto *value = getTypedData<std::deque<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = *value;
        if(currList.empty()){
            return RespMessage{RespType::Null};
        }
        std::string back_element = currList.back();
        currList.pop_back();
        if(currList.empty()){
            store.erase(it);
        }
        return RespMessage{RespType::Bulk,back_element};
    }
    return RespMessage{RespType::Null};
}
RespMessage handleLrange(const RespMessage &message){
    if(message.elements.size()!=4){
        return errorMessage("LRANGE should have 3 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it != store.end()){
        auto *value = getTypedData<std::deque<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::optional<int> start = getNumber(message.elements[2].data);
        std::optional<int> end = getNumber(message.elements[3].data);
        if(!start.has_value() || !end.has_value()){
            return errorMessage("Expiratoion number format wrong");
        }
        const std::deque<std::string> &currList = *value;
        int currSize = currList.size();
        if(start.value()<0){
            start.value() = currSize+start.value();
        }
        if(end.value()<0){
            end.value() = currSize+end.value();
        }
        end.value() = std::min(currSize-1,end.value());
        RespMessage result;
        result.type=RespType::Array;
        if(start.value()<0 || end.value()<0 || end.value()<start.value()){
            return result;
        }
        for(int idx=start.value();idx<=end.value();idx++){
            result.elements.push_back(RespMessage{RespType::Bulk,currList[idx]});
        }
        return result;
    }
    return RespMessage{RespType::Null};
}