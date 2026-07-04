#pragma once

#include"resp_message.h"
#include <string_view>
#include <optional>
std::optional<RespMessage> resp_parser(std::string_view encodedMessage,int &pos);