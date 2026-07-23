#ifndef ALTBOT_SYNC_CATCHUPCOMMAND_H
#define ALTBOT_SYNC_CATCHUPCOMMAND_H

#include "Chat.h"
#include "Player.h"

class CatchUpCommandHandler
{
public:
    static bool Handle(ChatHandler* handler, Optional<std::string> param);
};

class CatchUpCommand
{
public:
    static bool Validate(ChatHandler* handler, Player* player, Player* target);
};

#endif