#include "CatchUpCommand.h"

#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"

bool CatchUpCommandHandler::Handle(ChatHandler* const handler, Optional<std::string> param)
{
    Player* const player = handler->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("[catchup] Command must be executed by a logged-in player");
        return true;
    }

    ObjectGuid const selected = player->GetTarget();
    if (!selected)
    {
        handler->PSendSysMessage("[catchup] No target selected");
        return true;
    }

    Player* const target = ObjectAccessor::FindConnectedPlayer(selected);
    if (!target)
    {
        handler->PSendSysMessage("[catchup] Selected target is not a player character");
        return true;
    }

    CatchUpCommand::Validate(handler, player, target);

    return true;
}

bool CatchUpCommand::Validate(ChatHandler* const handler, Player* const player, Player* const target)
{
    PlayerbotAI* const botAI = sPlayerbotsMgr.GetPlayerbotAI(target);
    if (!botAI)
    {
        handler->PSendSysMessage("[catchup] Selected target is not a PlayerBot");
        return false;
    }

    if (botAI->GetMaster() != player)
    {
        handler->PSendSysMessage("[catchup] Selected target is not your PlayerBot");
        return false;
    }

    if (sRandomPlayerbotMgr.IsRandomBot(target))
    {
        handler->PSendSysMessage("[catchup] Selected target is a RandomBot");
        return false;
    }

    if (sRandomPlayerbotMgr.IsAddclassBot(target))
    {
        handler->PSendSysMessage("[catchup] Selected target is an AddclassBot");
        return false;
    }

    handler->PSendSysMessage("[catchup] Selected target {} is your altbot, ready to proceed", target->GetName());
    return true;
}