#include "CatchUpCommand.h"

#include "Log.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "QuestDef.h"
#include "RandomPlayerbotMgr.h"

bool CatchUpCommandHandler::Handle(ChatHandler* const handler, Optional<std::string> param)
{
    Player* const player = handler->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("[catchup] Command must be executed by a logged-in player");
        return true;
    }

    Player* const target = FindTarget(handler, player);
    if (!target)
    {
        return true;
    }

    Player* const bot = ValidateTarget(handler, player, target);
    if (!bot)
    {
        return true;
    }

    handler->PSendSysMessage("[catchup] Catching up altbot {} to player {}", bot->GetName(), player->GetName());

    CatchUpCommand catchUp(player, bot);
    catchUp.Execute();

    return true;
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

void CatchUpCommand::Execute()
{
    LOG_INFO("module", "[catchup] Catching up altbot {} to player {}", bot->GetName(), player->GetName());

    SetBotLevel();
    CompletePlayerQuests();
    CompleteClassQuests();
}

void CatchUpCommand::SetBotLevel()
{
    if (player->GetLevel() > bot->GetLevel())
    {
        LOG_INFO("module", "[catchup] Bot needs levelup to {}", player->GetLevel());
    }
    else
    {
        LOG_INFO("module", "[catchup] Bot already at or above player level");
    }
}

void CatchUpCommand::CompletePlayerQuests()
{
    const RewardedQuestSet& playerQuests = player->getRewardedQuests();

    for (RewardedQuestSet::const_iterator itr = playerQuests.begin(); itr != playerQuests.end(); ++itr)
    {
        uint32 const questId = *itr;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (quest->IsRepeatable() || quest->IsDailyOrWeekly() || quest->IsSeasonal() || quest->IsDFQuest() ||
            quest->IsPVPQuest())
        {
            LOG_INFO("module", "[catchup] Quest not required to catch up: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (!bot->SatisfyQuestClass(quest, false))
        {
            LOG_INFO("module", "[catchup] Bot not the right class for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (!bot->SatisfyQuestRace(quest, false))
        {
            LOG_INFO("module", "[catchup] Bot not the right race for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (bot->IsQuestRewarded(questId))
        {
            LOG_INFO("module", "[catchup] Bot already completed quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        LOG_INFO("module", "[catchup] Bot needs to complete quest: {} {}", questId, quest->GetTitle());
    }
}

void CatchUpCommand::CompleteClassQuests() { LOG_INFO("module", "[catchup] TODO: Class Quests"); }