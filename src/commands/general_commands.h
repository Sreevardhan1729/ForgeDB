#pragma once
#include "../resp_message.h"

RespMessage handlePing(const RespMessage &message);
RespMessage handleDel(const RespMessage &message);
RespMessage handleTtl(const RespMessage &message);
RespMessage handleExpire(const RespMessage &message);