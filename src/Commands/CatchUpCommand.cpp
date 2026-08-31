#include "CatchUpCommand.h"

#include "CatchUpCharacterManager.h"
#include "CatchUpQuestManager.h"
#include "Group.h"
#include "Log.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"

bool CatchUpCommandHandler::HandleDefaultCommand(ChatHandler* const handler, Optional<std::string> param)
{
    Player* const player = handler->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("[catchup] Command must be executed by a logged-in player");
        return true;
    }

    Player* const target = FindTarget(handler, player);
    if (!target)
        return true;

    Player* const bot = ValidateTarget(handler, player, target);
    if (!bot)
        return true;

    RunCatchUp(handler, player, bot);

    return true;
}

bool CatchUpCommandHandler::HandleAllCommand(ChatHandler* const handler, Optional<std::string> param)
{
    Player* const player = handler->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("[catchup] Command must be executed by a logged-in player");
        return true;
    }

    Group* const group = player->GetGroup();
    if (!group)
    {
        handler->PSendSysMessage("[catchup] You are not in a group");
        return true;
    }

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* groupMember = itr->GetSource();

        if (groupMember == player)
            continue;

        Player* const bot = ValidateTarget(handler, player, groupMember);
        if (!bot)
            continue;

        RunCatchUp(handler, player, bot);
    }

    return true;
}

void CatchUpCommandHandler::RunCatchUp(ChatHandler* const handler, Player* player, Player* bot)
{
    handler->PSendSysMessage("[catchup] Catching up altbot {} to player {}", bot->GetName(), player->GetName());

    CatchUpCommand catchUp(player, bot);

    catchUp.GiveLevel();

    if (catchUp.ShouldCompleteQuests())
        catchUp.CompleteQuests();
    else
        handler->PSendSysMessage("[catchup] {} should spend at least one talent point before completing quests",
                                 bot->GetName());
}

Player* CatchUpCommandHandler::FindTarget(ChatHandler* const handler, Player* const player)
{
    ObjectGuid const selected = player->GetTarget();
    if (!selected)
    {
        handler->PSendSysMessage("[catchup] You don't have a target");
        return nullptr;
    }

    Player* const target = ObjectAccessor::FindConnectedPlayer(selected);
    if (!target)
    {
        handler->PSendSysMessage("[catchup] Your target is not a player character");
        return nullptr;
    }

    return target;
}

Player* CatchUpCommandHandler::ValidateTarget(ChatHandler* const handler, Player* const player, Player* const target)
{
    PlayerbotAI* const botAI = sPlayerbotsMgr.GetPlayerbotAI(target);
    if (!botAI)
    {
        handler->PSendSysMessage("[catchup] {} is not a PlayerBot", target->GetName());
        return nullptr;
    }

    if (botAI->GetMaster() != player)
    {
        handler->PSendSysMessage("[catchup] {} is not your PlayerBot", target->GetName());
        return nullptr;
    }

    if (sRandomPlayerbotMgr.IsRandomBot(target))
    {
        handler->PSendSysMessage("[catchup] {} is a RandomBot", target->GetName());
        return nullptr;
    }

    if (sRandomPlayerbotMgr.IsAddclassBot(target))
    {
        handler->PSendSysMessage("[catchup] {} is an AddclassBot", target->GetName());
        return nullptr;
    }

    return target;
}

CatchUpCommand::CatchUpCommand(Player* player, Player* bot) : player(player), bot(bot) {}

void CatchUpCommand::GiveLevel()
{
    CatchUpCharacterManager characterManager(bot);

    if (player->GetLevel() > bot->GetLevel())
    {
        LOG_INFO("module", "[catchup] Bot needs levelup from {} to {}", bot->GetLevel(), player->GetLevel());
        characterManager.GiveLevel(player);
    }
    else
        LOG_INFO("module", "[catchup] Bot already at or above player level");

    characterManager.GiveAbilities();
    characterManager.GiveBags(player);
}

void CatchUpCommand::CompleteQuests()
{
    CatchUpQuestManager questManager(sPlayerbotsMgr.GetPlayerbotAI(bot));
    questManager.AddPlayerQuests(player);
    questManager.AddClassQuests();
    questManager.SatisfyPreQuests();
    CompleteQuestResult result = questManager.CompleteQuests();

    if (result == QUEST_ERR_OK)
        LOG_INFO("module", "[catchup] Quests completed successfully");
    else
        LOG_INFO("module", "[catchup] Failed to complete quests");
}

bool CatchUpCommand::ShouldCompleteQuests()
{
    uint8 talents[3] = {0, 0, 0};
    bot->GetTalentTreePoints(talents);
    return talents[0] > 0 || talents[1] > 0 || talents[2] > 0;
}