#include"utils.h"
#include "resp_message.h"
#include <cctype>
#include <optional>

std::optional<int> getNumber(std::string_view time){
    if(time.empty() || time == "-"){
        return std::nullopt;
    }
    bool negative = false;
    int pos=0;
    int ans=0;
    if(time[pos]=='-'){
        pos++;
        negative=true;
    }
    while(pos<time.size()){
        if(!std::isdigit(time[pos])) return std::nullopt;
        ans = (ans*10)+(int)(time[pos++]-'0');
    }
    return negative ? -ans : ans;
}

RespMessage errorMessage(std::string error){
    return RespMessage{RespType::Error,error};
}