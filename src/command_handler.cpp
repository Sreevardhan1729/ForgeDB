#include "command_handler.h"
#include "resp_message.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
struct Store{
    std::string data;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;
};
static std::unordered_map<std::string, Store> store;
int getNumber(std::string_view time){
    int val = 0;
    for(int i=0;i<time.size();i++){
        if(!isdigit(time[i])) return -1;
        val = (val*10)+(int)(time[i]-'0');
    }
    return val;
}
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
    if(message.elements.size()!=3 && (message.elements.size()!=5 || message.elements[3].data!="EX")){
        return RespMessage{RespType::Error,"SET accepts only 2 or 4 Arguments"};
    }
    if(message.elements.size()==5){
        int val = getNumber(message.elements[4].data);
        if(val==-1){
            return RespMessage{RespType::Error,"Expiratoion value format wrong"};
        }
        if(val!=0){
            std::chrono::steady_clock::time_point expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(val);
            store[message.elements[1].data] = Store{message.elements[2].data,expiresAt};
        }
        else{
            return RespMessage{RespType::Error,"Invalid expiration value"};
        }
    }
    else{
        store[message.elements[1].data] = Store{message.elements[2].data,std::nullopt};
    }
    return RespMessage{RespType::Simple,"OK"};
}
RespMessage handleGet(const RespMessage &message){
    if(message.elements.size()!=2){
        return RespMessage{RespType::Error,"GET accepts only 1 Argument"};
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        if(it->second.expiresAt.has_value() && *it->second.expiresAt <= std::chrono::steady_clock::now()){
            store.erase(it);
            return RespMessage{RespType::Null};
        }
        return RespMessage{RespType::Bulk,it->second.data};
            
    }
    return RespMessage{RespType::Null};
}
RespMessage handleDel(const RespMessage &message){
    if(message.elements.size()<2){
        return RespMessage{RespType::Error,"DEL should have atleast 1 Argument"};
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
        return RespMessage{RespType::Error,"TTL should have atleast 1 Argument"};
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
        return RespMessage{RespType::Error,"EXPIRE should have atleast 1 Argument"};
    }
    auto it = store.find(message.elements[1].data);
    if(it!=store.end()){
        int val = getNumber(message.elements[2].data);
        if(val==-1){
            return RespMessage{RespType::Error,"Expiratoion number format wrong"};
        }
        std::chrono::steady_clock::time_point expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(val);
        it->second.expiresAt = expiresAt;
        return RespMessage{RespType::Integer,"1"};
    }
    return RespMessage{RespType::Integer,"0"};
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
};

RespMessage handleCommand(const RespMessage &message){
    std::string commandName = message.elements[0].data;
    std::transform(commandName.begin(),commandName.end(),commandName.begin(),::toupper);
    auto it = dispatchTable.find(commandName);
    if(it!=dispatchTable.end()){
        return it->second(message);
    }
    else{
        return RespMessage{RespType::Error,"Invlaid Dispatcher command"};
    }
}