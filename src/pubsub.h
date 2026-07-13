#pragma once
#include "resp_message.h"
RespMessage handleSubscribe(const RespMessage &message, int clientFd);
RespMessage handleUnSubscribe(const RespMessage &message,int clientFd);
RespMessage handlePublish(const RespMessage &message,int clientFd);
RespMessage unsubscribeClient(int clientFd);