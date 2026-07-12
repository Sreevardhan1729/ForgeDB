#include <chrono>
#include <cstdlib>
#include <deque>
#include<gtest/gtest.h>
#include <optional>
#include <string>
#include <unordered_map>
#include "../src/datastore.h"
#include "../src/resp_parser.h"
#include"../src/resp_serializer.h"
#include "../src/command_handler.h"
TEST(RespTests, parseBulkTest){
    int pos =0;
    RespMessage output;
    output.data = "foo";
    output.type = RespType::Bulk;
    EXPECT_EQ(resp_parser("$3\r\nfoo\r\n", pos),output);
    
}
TEST(RespTests, parseArrayTest){
    int pos=0;
    RespMessage output1;
    RespMessage temp1,temp2;
    temp1.type=RespType::Bulk;
    temp1.data = "foo";
    temp2.type= RespType::Bulk;
    temp2.data="bar";
    output1.type = RespType::Array;
    output1.elements = {temp1,temp2};
    EXPECT_EQ(resp_parser("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", pos), output1);
}

TEST(RespTests, roundTripTest){
    RespMessage test;
    test.type = RespType::Array;
    test.data = "";
    test.elements = {RespMessage{RespType::Bulk,"hello",{}},RespMessage{RespType::Bulk,"how",{}},RespMessage{RespType::Bulk,"are",{}},RespMessage{RespType::Bulk,"you",{}}};
    int pos=0;
    EXPECT_EQ(resp_parser(resp_serializer(test),pos),test);
}


TEST(RespTests, parsePartialData){
    int pos=0;
    RespMessage output1;
    RespMessage temp1,temp2;
    temp1.type=RespType::Bulk;
    temp1.data = "foo";
    temp2.type= RespType::Bulk;
    temp2.data="bar";
    output1.type = RespType::Array;
    output1.elements = {temp1,temp2};
    EXPECT_EQ(resp_parser("*2\r\n$3\r\nfoo\r\n$3\r\nbar", pos), std::nullopt);
}

TEST(storeTests, serializeTestString){
    RespMessage input;
    input.type=RespType::Array;
    input.elements.push_back(RespMessage{RespType::Bulk,"set"});
    input.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    input.elements.push_back(RespMessage{RespType::Bulk,"bar"});
    handleCommand(input);
    EXPECT_EQ(serializeStore(),"3\r\nfoo\r\nS3\r\nbar\r\n");
    clearStore();
}

TEST(storeTests, serializeTestList){
    RespMessage input;
    input.type=RespType::Array;
    input.elements.push_back(RespMessage{RespType::Bulk,"Lpush"});
    input.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    input.elements.push_back(RespMessage{RespType::Bulk,"a"});
    input.elements.push_back(RespMessage{RespType::Bulk,"b"});
    input.elements.push_back(RespMessage{RespType::Bulk,"c"});
    handleCommand(input);
    EXPECT_EQ(serializeStore(),"3\r\nfoo\r\nL3\r\n1\r\nc\r\n1\r\nb\r\n1\r\na\r\n");
    clearStore();
}

TEST(storeTests, rouondTripString){
    RespMessage input;
    input.type=RespType::Array;
    input.elements.push_back(RespMessage{RespType::Bulk,"set"});
    input.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    input.elements.push_back(RespMessage{RespType::Bulk,"bar"});
    handleCommand(input);
    std::unordered_map<std::string, Store> temp = store;
    std::string data = serializeStore();
    clearStore();
    int des = deserializeStore(data);
    EXPECT_EQ(store,temp);
    clearStore();
}
TEST(storeTests, rouondTripList){
    RespMessage input;
    input.type=RespType::Array;
    input.elements.push_back(RespMessage{RespType::Bulk,"Lpush"});
    input.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    input.elements.push_back(RespMessage{RespType::Bulk,"a"});
    input.elements.push_back(RespMessage{RespType::Bulk,"b"});
    input.elements.push_back(RespMessage{RespType::Bulk,"c"});
    handleCommand(input);
    std::unordered_map<std::string, Store> temp = store;
    std::string data = serializeStore();
    clearStore();
    int des = deserializeStore(data);
    EXPECT_EQ(store,temp);
    clearStore();
}

TEST(storeTests, rouondTripListWithTIme){
    RespMessage input;
    input.type=RespType::Array;
    input.elements.push_back(RespMessage{RespType::Bulk,"Lpush"});
    input.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    input.elements.push_back(RespMessage{RespType::Bulk,"a"});
    input.elements.push_back(RespMessage{RespType::Bulk,"b"});
    input.elements.push_back(RespMessage{RespType::Bulk,"c"});
    RespMessage addTime;
    addTime.type = RespType::Array;
    addTime.elements.push_back(RespMessage{RespType::Bulk,"expire"});
    addTime.elements.push_back(RespMessage{RespType::Bulk,"foo"});
    addTime.elements.push_back(RespMessage{RespType::Bulk,"30"});
    handleCommand(input);
    handleCommand(addTime);
    std::unordered_map<std::string, Store> temp = store;
    std::string data = serializeStore();
    clearStore();
    int des = deserializeStore(data);
    auto it = store.find("foo");
    auto it2 = temp.find("foo");
    EXPECT_NE(it,store.end());
    auto* list = getTypedData<std::deque<std::string>>(it->second);
    auto* list2 = getTypedData<std::deque<std::string>>(it2->second);
    EXPECT_NE(list,nullptr);
    EXPECT_NE(list2,nullptr);
    EXPECT_EQ(*list, *list2);

    auto diff = *(it->second.expiresAt) - *(it2->second.expiresAt);
    auto absDiff = std::chrono::abs(diff);
    EXPECT_LT(absDiff,std::chrono::seconds(2));
    clearStore();
}