#include "CatchUpCommand.h"

#include "CatchUpQuestManager.h"
#include "Log.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"

bool CatchUpCommandHandler::HandleDefaultCommand(ChatHandler* const handler, Optional<std::string> param)
{
    CatchUpCommand* catchUp = CommandFactory(handler);
    if (!catchUp)
        return true;

    catchUp->SetBotLevel();
    catchUp->CompleteQuests();

    delete catchUp;

    return true;
}

bool CatchUpCommandHandler::HandleLevelCommand(ChatHandler* const handler, Optional<std::string> param)
{
    CatchUpCommand* catchUp = CommandFactory(handler);
    if (!catchUp)
        return true;

    catchUp->SetBotLevel();

    delete catchUp;

    return true;
}

CatchUpCommand* CatchUpCommandHandler::CommandFactory(ChatHandler* const handler)
{
    Player* const player = handler->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("[catchup] Command must be executed by a logged-in player");
        return nullptr;
    }

    Player* const target = FindTarget(handler, player);
    if (!target)
    {
        return nullptr;
    }

    Player* const bot = ValidateTarget(handler, player, target);
    if (!bot)
    {
        return nullptr;
    }

    handler->PSendSysMessage("[catchup] Catching up altbot {} to player {}", bot->GetName(), player->GetName());

    return new CatchUpCommand(player, bot);
}

Player* CatchUpCommandHandler::FindTarget(ChatHandler* const handler, Player* const player)
{
    ObjectGuid const selected = player->GetTarget();
    if (!selected)
    {
        handler->PSendSysMessage("[catchup] No target selected");
        return nullptr;
    }

    Player* const target = ObjectAccessor::FindConnectedPlayer(selected);
    if (!target)
    {
        handler->PSendSysMessage("[catchup] Selected target is not a player character");
        return nullptr;
    }

    return target;
}

Player* CatchUpCommandHandler::ValidateTarget(ChatHandler* const handler, Player* const player, Player* const target)
{
    PlayerbotAI* const botAI = sPlayerbotsMgr.GetPlayerbotAI(target);
    if (!botAI)
    {
        handler->PSendSysMessage("[catchup] Selected target is not a PlayerBot");
        return nullptr;
    }

    if (botAI->GetMaster() != player)
    {
        handler->PSendSysMessage("[catchup] Selected target is not your PlayerBot");
        return nullptr;
    }

    if (sRandomPlayerbotMgr.IsRandomBot(target))
    {
        handler->PSendSysMessage("[catchup] Selected target is a RandomBot");
        return nullptr;
    }

    if (sRandomPlayerbotMgr.IsAddclassBot(target))
    {
        handler->PSendSysMessage("[catchup] Selected target is an AddclassBot");
        return nullptr;
    }

    return target;
}

CatchUpCommand::CatchUpCommand(Player* player, Player* bot) : player(player), bot(bot) {}

void CatchUpCommand::SetBotLevel()
{
    if (player->GetLevel() > bot->GetLevel())
    {
        LOG_INFO("module", "[catchup] Bot needs levelup from {} to {}", bot->GetLevel(), player->GetLevel());

        bot->GiveLevel(player->GetLevel());
        bot->SetUInt32Value(PLAYER_XP, 0);
    }
    else
    {
        LOG_INFO("module", "[catchup] Bot already at or above player level");
    }
}

void CatchUpCommand::CompleteQuests()
{
    CatchUpQuestManager questManager(bot, sPlayerbotsMgr.GetPlayerbotAI(bot));
    questManager.AddPlayerQuests(player);
    questManager.AddClassQuests();
    questManager.SatisfyPreQuests();
    CompleteQuestResult result = questManager.CompleteQuests();

    if (result == QUEST_ERR_OK)
        LOG_INFO("module", "[catchup] Quests completed successfully");
    else
        LOG_INFO("module", "[catchup] Failed to complete quests");
}