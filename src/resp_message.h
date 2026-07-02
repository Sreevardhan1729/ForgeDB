#pragma once

#include<string>
#include<vector>
enum class RespType {
    Simple,
    Bulk,
    Error,
    Array
};
struct RespMessage {
    RespType type;
    std::string data;
    std::vector<RespMessage> elements;

    bool operator==(const RespMessage& other) const {
        return (type == other.type && data == other.data && elements == other.elements);
    }
};