#ifndef ALTBOT_SYNC_CATCHUPCOMMAND_H
#define ALTBOT_SYNC_CATCHUPCOMMAND_H

#include "Chat.h"
#include "Player.h"

class CatchUpCommandHandler
{
public:
    static bool Handle(ChatHandler* handler, Optional<std::string> param);

private:
    static Player* FindTarget(ChatHandler* handler, Player* player);
    static Player* ValidateTarget(ChatHandler* handler, Player* player, Player* target);
};

class CatchUpCommand
{
public:
    CatchUpCommand(Player* player, Player* bot);

    void Execute();

private:
    void SetBotLevel();
    void CompletePlayerQuests();
    void CompleteClassQuests();

    Player* player;
    Player* bot;
};

#endif