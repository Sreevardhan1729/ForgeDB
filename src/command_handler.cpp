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
#include <variant>
struct Store{
    std::variant<std::string,std::deque<std::string>> data;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
};
static std::unordered_map<std::string, Store> store;
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
bool checkVariantString(std::variant<std::string,std::deque<std::string>> &data){
    if(std::holds_alternative<std::string>(data)){
        return true;
    }
    return false;
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
        if(!checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        return RespMessage{RespType::Bulk,std::get<std::string>(it->second.data)};
            
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
        if(checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = std::get<std::deque<std::string>>(it->second.data);
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
        if(checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string>& currList = std::get<std::deque<std::string>>(it->second.data);
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
        if(checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = std::get<std::deque<std::string>>(it->second.data);
        if(currList.empty()){
            return RespMessage{RespType::Null};
        }
        std::string value = currList.front();
        currList.pop_front();
        if(currList.empty()){
            store.erase(it);
        }
        return RespMessage{RespType::Bulk,value};
    }
    return RespMessage{RespType::Null};
}
RespMessage handleRpop(const RespMessage &message){
    if(message.elements.size()!=2){
        return errorMessage("RPOP should have 2 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        if(checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::deque<std::string> &currList = std::get<std::deque<std::string>>(it->second.data);
        if(currList.empty()){
            return RespMessage{RespType::Null};
        }
        std::string value = currList.back();
        currList.pop_back();
        if(currList.empty()){
            store.erase(it);
        }
        return RespMessage{RespType::Bulk,value};
    }
    return RespMessage{RespType::Null};
}
RespMessage handleLrange(const RespMessage &message){
    if(message.elements.size()!=4){
        return errorMessage("LRANGE should have 3 arguments");
    }
    auto it = store.find(message.elements[1].data);
    if(it != store.end()){
        if(checkVariantString(it->second.data)){
            return errorMessage("WRONGTYPE Operation against a key holding the wrong kind of value");
        }
        std::optional<int> start = getNumber(message.elements[2].data);
        std::optional<int> end = getNumber(message.elements[3].data);
        if(!start.has_value() || !end.has_value()){
            return errorMessage("Expiratoion number format wrong");
        }
        std::deque<std::string> currList = std::get<std::deque<std::string>>(it->second.data);
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
    {"LRANGE",handleLrange}
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