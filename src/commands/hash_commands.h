#pragma once
#include "../resp_message.h"

RespMessage handleHset(const RespMessage &message);
RespMessage handleHget(const RespMessage &message);
RespMessage handleHdel(const RespMessage &message);
RespMessage handleHgetall(const RespMessage &message);