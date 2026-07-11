#pragma once

#include"resp_message.h"
#include <optional>
#include <string>
#include <string_view>
std::optional<int> getNumber(std::string_view time);
RespMessage errorMessage(std::string error);