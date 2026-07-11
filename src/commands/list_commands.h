#pragma once
#include "../resp_message.h"

RespMessage handleLpush(const RespMessage &message);
RespMessage handleRpush(const RespMessage &message);
RespMessage handleLpop(const RespMessage &message);
RespMessage handleRpop(const RespMessage &message);
RespMessage handleLrange(const RespMessage &message);