#include<gtest/gtest.h>
#include <optional>
#include "../src/resp_parser.h"
#include"../src/resp_serializer.h"
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
