#include "datastore.h"
#include <chrono>
#include <string>
#include <unordered_map>

std::unordered_map<std::string, Store> store;

void clearStore(){
    store.clear();
}

void clearExpiredKeys(){
    for(auto it=store.begin();it!=store.end();){
        if(it->second.expiresAt.has_value() && it->second.expiresAt<=std::chrono::steady_clock::now()){
            it = store.erase(it);
        }
        else{
            ++it;
        }
    }
}