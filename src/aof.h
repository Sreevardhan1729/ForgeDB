#pragma once
#include <string>
void writeToAof(std::string data);
void initAof(const char* filename);
void fsyncAof();
void replayAof();