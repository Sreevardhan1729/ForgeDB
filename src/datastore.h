#pragma  once

#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
using DataVariant = std::variant<
    std::string,
    std::deque<std::string>,
    std::unordered_set<std::string>,
    std::unordered_map<std::string, std::string>
>;
struct Store{
    DataVariant data;
    std::optional<std::chrono::steady_clock::time_point> expiresAt;

    bool operator==(const Store& other) const {
        return data == other.data && expiresAt == other.expiresAt;
    }
};

extern std::unordered_map<std::string, Store> store;


template<typename T>
T* getTypedData(Store &s){
    return std::get_if<T>(&s.data);
}
void clearStore();
void clearExpiredKeys();