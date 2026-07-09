#include "command_handler.h"
#include "resp_message.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>

using DataVariant = std::variant<
    std::string,
    std::deque<std::string>,
    std::unordered_map<std::string, std::string>,
    std::unordered_set<std::string>
>;
struct Store{
    DataVariant data;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
};
static std::unordered_map<std::string, Store> store;
template<typename T>
T* getTypedData(Store &store_entry){
    return std::get_if<T>(&store_entry.data);
}
std::optional<int> getNumber(std::string_view time){
    int val = 0;
    int i=0;
    bool negative = false;
    if(time[0]=='-'){
        negative=true;
        i++;
    }
    for(;i<time.size();i++){
        if(!isdigit(time[i])) return std::nullopt;
        val = (val*10)+(int)(time[i]-'0');
    }
    return negative ? -val : val;
}
RespMessage errorMessage(std::string error){
    return RespMessage{RespType::Error,error};
}
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
void clearStore(){
    store.clear();
}

void cleanUpExpiredKeys(){
    for(auto it = store.begin(); it!=store.end();){
        if(it->second.expiresAt.has_value() && *it->second.expiresAt <= std::chrono::steady_clock::now()){
            it = store.erase(it);
        }
        else{
            ++it;
        }
    }
}

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