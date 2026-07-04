#include<vector>
#include<string>
#include "resp_serializer.h"
#include "resp_message.h"
std::string resp_serializer(const RespMessage &messages){
    const std::string &message = messages.data;
    int messageSize = message.length();
    std::string encodedMessage;
    if(messages.type==RespType::Null){
        return "$-1\r\n";
    }
    if(messages.type==RespType::Simple){
        if(messageSize==0){
            return "+\r\n";
        }
        encodedMessage.push_back('+');
        encodedMessage+=message;
        encodedMessage.push_back('\r');
        encodedMessage.push_back('\n');
    }
    else if(messages.type==RespType::Error){
        if(messageSize==0){
            return "-\r\n";
        }
        encodedMessage.push_back('-');
        encodedMessage+=message;
        encodedMessage.push_back('\r');
        encodedMessage.push_back('\n');
    }
    else if(messages.type==RespType::Bulk){
        if(messageSize==0){
            return "$0\r\n\r\n";
        }
        encodedMessage.push_back('$');
        encodedMessage+=std::to_string(messageSize);
        encodedMessage.push_back('\r');
        encodedMessage.push_back('\n');
        encodedMessage+=message;
        encodedMessage.push_back('\r');
        encodedMessage.push_back('\n');
    }
    else if(messages.type == RespType::Array){
        const std::vector<RespMessage> &elements = messages.elements;
        int size = elements.size();
        if(size==0){
            return "*0\r\n";
        }
        encodedMessage.push_back('*');
        encodedMessage+=std::to_string(size);
        encodedMessage.push_back('\r');
        encodedMessage.push_back('\n');
        for(const RespMessage &message: elements){
            
            encodedMessage += resp_serializer(message);
        }
    }
    return encodedMessage;
}
