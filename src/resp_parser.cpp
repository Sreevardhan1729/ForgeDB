#include "resp_parser.h"
#include "resp_message.h"
#include <optional>
#include<string>
#include <string_view>
#include<vector>
bool checkIfEnd(std::string_view message,int &idx){
    if(idx+1 < message.size() && message[idx]=='\r' && message[idx+1]=='\n'){
        idx+=2;
        return true;
    }
    return false;
}
int getNumber(std::string_view message,int &idx){
    int num=0;
    int messageSize = message.size();
    bool isMessageEnd=false;
    while(idx<messageSize){
        if(checkIfEnd(message, idx)){
            isMessageEnd=true;
            break;
        }
        num = num*10 + (message[idx]-'0');
        idx++;
    }
    return isMessageEnd ? num : -1;
}
bool checkNegative(std::string_view message,int &idx){
    if(idx+1<message.size() && message[idx]=='-' && message[idx+1]=='1'){
        idx+=2;
        if(checkIfEnd(message,idx)){
            return true;
        }
        else{
            idx-=2;
        }
    }
    return false;
}
std::optional<RespMessage> resp_parser(std::string_view encodedMessage,int &pos){
    int messageSize = encodedMessage.size();
    if(pos >= messageSize){
        return std::nullopt;
    }
    bool isMessageEnd=false;
    RespMessage result; 
    if(encodedMessage[pos]=='+' || encodedMessage[pos]=='-'){
        bool isError = encodedMessage[pos]=='-';
        pos++;
        std::string decodedMessage;
       while(pos<messageSize){
            if(checkIfEnd(encodedMessage, pos)){
                isMessageEnd = true;
                break;
            }
            decodedMessage.push_back(encodedMessage[pos]);
            pos++;
        }
        if(!isMessageEnd){
            return std::nullopt;
        }
        RespMessage currMessage;
        currMessage.data = decodedMessage;
        currMessage.type = isError ? RespType::Error : RespType::Simple;
        result=currMessage;
    }
    else if(encodedMessage[pos]=='$'){
        std::string decodedMessage;
        pos++;
        if(checkNegative(encodedMessage,pos)){
            return RespMessage{RespType::Null};
        }
        int count = getNumber(encodedMessage, pos);
        if(count==-1){
            return std::nullopt;
        }
        RespType type = RespType::Bulk;
        if(messageSize-pos < count+2){
            return std::nullopt;
        }
        else {
            while(count>0){
                decodedMessage.push_back(encodedMessage[pos]);
                pos++;
                count--;
            }
            pos+=2;
        }
        RespMessage currMessage;
        currMessage.data = decodedMessage;
        currMessage.type = type;
        result=currMessage;
    }
    else if(encodedMessage[pos]=='*'){
        pos++;
        int arraySize = getNumber(encodedMessage, pos);
        if(arraySize==-1){
            return std::nullopt;
        }
        for(int cnt=0;cnt<arraySize;cnt++){
            std::optional<RespMessage> currrentResult = resp_parser(encodedMessage,pos);
            if(currrentResult.has_value()){
                result.elements.push_back(currrentResult.value());
            }
            else{
                return std::nullopt;
            }
        }
        result.type = RespType::Array;
    }
    return result;
}
