#ifndef ALTBOT_SYNC_CATCHUPCOMMAND_H
#define ALTBOT_SYNC_CATCHUPCOMMAND_H

#include "Chat.h"
#include "Player.h"

class CatchUpCommand;

class CatchUpCommandHandler
{
public:
    static bool HandleDefaultCommand(ChatHandler* handler, Optional<std::string> param);
    static bool HandleLevelCommand(ChatHandler* handler, Optional<std::string> param);

private:
    static CatchUpCommand* CommandFactory(ChatHandler* handler);
    static Player* FindTarget(ChatHandler* handler, Player* player);
    static Player* ValidateTarget(ChatHandler* handler, Player* player, Player* target);
};

class CatchUpCommand
{
public:
    CatchUpCommand(Player* player, Player* bot);

    void GiveLevel();
    void CompleteQuests();

    bool ShouldCompleteQuests();

private:
    Player* player;
    Player* bot;
};

#endif