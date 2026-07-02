#include "resp_parser.h"
#include "resp_message.h"

#include<string>
#include <string_view>
#include<vector>
bool checkIfEnd(std::string_view message,int idx){
    if(idx+1 < message.size() && message[idx]=='\r' && message[idx+1]=='\n'){
        return true;
    }
    return false;
}
int getNumber(std::string_view message,int &idx){
    int num=0;
    while(true){
        if(checkIfEnd(message, idx)){
            break;
        }
        num = num*10 + (message[idx]-'0');
        idx++;
    }
    return num;
}
RespMessage resp_parser(std::string_view encodedMessage,int &pos){
    if(pos == encodedMessage.size()){
        return {};
    }

    RespMessage result; 
    if(encodedMessage[pos]=='+' || encodedMessage[pos]=='-'){
        bool isError = encodedMessage[pos]=='-';
        pos++;
        std::string decodedMessage;
       while(true){
            if(checkIfEnd(encodedMessage, pos)){
                pos+=2;
                break;
            }
            decodedMessage.push_back(encodedMessage[pos]);
            pos++;
        }
        RespMessage currMessage;
        currMessage.data = decodedMessage;
        currMessage.type = isError ? RespType::Error : RespType::Simple;
        result=currMessage;
    }
    else if(encodedMessage[pos]=='$'){
        std::string decodedMessage;
        pos++;
        int count = getNumber(encodedMessage, pos);
        pos+=2;
        RespType type = RespType::Bulk;
        if(encodedMessage.size()-pos < count){
            decodedMessage = "-ERR Message length incorrect";
            type = RespType::Error;
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
        pos+=2;
        for(int cnt=0;cnt<arraySize;cnt++){
            RespMessage currrentResult = resp_parser(encodedMessage,pos);
            result.elements.push_back(currrentResult);
        }
        result.type = RespType::Array;
    }
    return result;
}
