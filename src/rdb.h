#pragma once

#include "datastore.h"
#include <string>
std::string serializeStore();
int deserializeStore(std::string &data);
int saveRDB(const char* filename);
int loadRDB(const char* filename);