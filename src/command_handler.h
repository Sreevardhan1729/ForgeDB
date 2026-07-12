#include"resp_message.h"
#include"utils.h"
#include"datastore.h"
#include "commands/general_commands.h"
#include "commands/string_commands.h"
#include "commands/list_commands.h"
#include "commands/hash_commands.h"
#include "commands/set_commands.h"
#include "rdb.h"

RespMessage handleCommand(const RespMessage &message);