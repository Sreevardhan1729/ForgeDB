#pragma once
#include "../resp_message.h"

RespMessage handleSadd(const RespMessage &message);
RespMessage handleSmembers(const RespMessage &message);
RespMessage handleSismember(const RespMessage &message);
RespMessage handleSrem(const RespMessage &message);
RespMessage handleSinter(const RespMessage &message);