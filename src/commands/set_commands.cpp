#include "set_commands.h"
#include "../datastore.h"
#include "../utils.h"

RespMessage handleSadd(const RespMessage &message){
    if(message.elements.size()<3){
        return errorMessage("SADD need to have atleast 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    int count=0;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_set<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        for(int idx = 2;idx<message.elements.size();idx++){
            auto fieldIt = value->find(message.elements[idx].data);
            if(fieldIt==value->end()){
                count++;
                value->insert(message.elements[idx].data);
            }
        }
    }
    else{
        std::unordered_set<std::string> value;
        for(int idx = 2;idx<message.elements.size();idx++){
            auto fieldIt = value.find(message.elements[idx].data);
            if(fieldIt==value.end()){
                count++;
                value.insert(message.elements[idx].data);
            }
        }
        store[message.elements[1].data] = {value};
    }
    return RespMessage{RespType::Integer,std::to_string(count)};
}
RespMessage handleSmembers(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("SMEMBERS need to have exactly 1 arguments");
    }
    auto it = store.find(message.elements[1].data);
    RespMessage result;
    result.type=RespType::Array;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_set<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        for(auto mapIt : *value){
            result.elements.push_back(RespMessage{RespType::Bulk,mapIt});
        }
    }
    return result;
}
RespMessage handleSismember(const RespMessage &message){
    if(message.elements.size()!=3){
        return errorMessage("SISMEMBER need to have exactly 3 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_set<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        auto fieldIt = value->find(message.elements[2].data);
        if(fieldIt!=value->end()){
            return RespMessage{RespType::Integer,"1"};
        }
    }
    return RespMessage{RespType::Integer,"0"};
}
RespMessage handleSrem(const RespMessage &message){
    if(message.elements.size()<3){
        return errorMessage("SREM need to have atleast 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    int count=0;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_set<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        for(int idx=2;idx<message.elements.size();idx++){
            auto fieldIt = value->find(message.elements[idx].data);
            if(fieldIt!=value->end()){
                value->erase(fieldIt);
                count++;
            }
        }
        if(value->size()==0){
            store.erase(it);
        }
    }
    return RespMessage{RespType::Integer,std::to_string(count)};
}
RespMessage handleSinter(const RespMessage &message){
    if(message.elements.size()<=2){
        return errorMessage("SINTER need to have atleast 1 arguments");
    }
    auto it = store.find(message.elements[1].data);
    RespMessage result;
    result.type=RespType::Array;
    if(it!=store.end()){
        auto* value = getTypedData<std::unordered_set<std::string>>(it->second);
        if(!value){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::unordered_set<std::string> baseSet = *value;
        for(int idx=2;idx<message.elements.size();idx++){
            auto fieldIt = store.find(message.elements[idx].data);
            if(fieldIt==store.end()){
                baseSet.clear();
                break;
            }
            auto* value = getTypedData<std::unordered_set<std::string>>(fieldIt->second);
            if(!value){
                return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
            }
            for(auto it2 = baseSet.begin();it2!=baseSet.end();){
                if(value->find(*it2)==value->end()){
                    it2 = baseSet.erase(it2);
                }
                else{
                    it2++;
                }
            }
        }
        for(std::string element:baseSet){
            result.elements.push_back(RespMessage{RespType::Bulk,element});
        }
    }
    return result;
}