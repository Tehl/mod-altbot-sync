#include "CatchUpQuestManager.h"

#include "ObjectMgr.h"
#include "QuestDef.h"
#include "SpellMgr.h"

QuestIdSet CatchUpQuestManager::classQuestIds;

void CatchUpQuestManager::Init()
{
    ObjectMgr::QuestMap const& questTemplates = sObjectMgr->GetQuestTemplates();
    for (ObjectMgr::QuestMap::const_iterator i = questTemplates.begin(); i != questTemplates.end(); ++i)
    {
        uint32 questId = i->first;
        Quest const* quest = i->second;

        if (!quest->GetRequiredClasses() || quest->IsRepeatable())
            continue;

        if (quest->GetRewSpellCast() > 0)
        {
            int32 spellId = quest->GetRewSpellCast();
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;
        }
        else if (quest->GetRewSpell() > 0)
        {
            int32 spellId = quest->GetRewSpell();
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo)
                continue;
        }

        classQuestIds.insert(questId);
    }
}

void CatchUpQuestManager::SatisfyPreQuests(QuestIdSet& questSet)
{
    QuestIdSet preQuests;
    for (QuestIdSet::const_iterator itr_i = questSet.begin(); itr_i != questSet.end(); ++itr_i)
    {
        uint32 const questId = *itr_i;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        for (Quest::PrevQuests::const_iterator itr_j = quest->prevQuests.begin(); itr_j != quest->prevQuests.end();
             ++itr_j)
        {
            uint32 prevId = abs(*itr_j);
            if (questSet.find(prevId) == questSet.end())
            {
                LOG_INFO("module", "[catchup] Quest {} {} needs pre-quest: {} {}", questId, quest->GetTitle(), prevId,
                         sObjectMgr->GetQuestTemplate(prevId)->GetTitle());
                preQuests.insert(prevId);
            }
            else
            {
                LOG_TRACE("module", "[catchup] Quest {} {} pre-quest {} {} already satisfied", questId,
                          quest->GetTitle(), prevId, sObjectMgr->GetQuestTemplate(prevId)->GetTitle());
            }
        }
    }

    if (preQuests.size() > 0)
    {
        LOG_TRACE("module", "[catchup] Found {} pre-quests to add to the set", preQuests.size());

        SatisfyPreQuests(preQuests);
        questSet.merge(preQuests);
    }
    else
    {
        LOG_INFO("module", "[catchup] All pre-quests satisfied");
    }
}

CatchUpQuestManager::CatchUpQuestManager(Player* bot) : bot(bot) {}

void CatchUpQuestManager::AddPlayerQuests(Player* player)
{
    const RewardedQuestSet& playerQuests = player->getRewardedQuests();

    for (RewardedQuestSet::const_iterator itr = playerQuests.begin(); itr != playerQuests.end(); ++itr)
    {
        uint32 const questId = *itr;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (quest->IsRepeatable() || quest->IsDailyOrWeekly() || quest->IsSeasonal() || quest->IsDFQuest() ||
            quest->IsPVPQuest() || quest->GetRequiredClasses())
        {
            LOG_TRACE("module", "[catchup] Quest not required to catch up: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (!bot->SatisfyQuestRace(quest, false))
        {
            LOG_TRACE("module", "[catchup] Bot not the right race for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (bot->IsQuestRewarded(questId))
        {
            LOG_TRACE("module", "[catchup] Bot already completed quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        LOG_INFO("module", "[catchup] Bot needs to complete player quest: {} {}", questId, quest->GetTitle());
        requiredQuestIds.insert(questId);
    }
}

void CatchUpQuestManager::AddClassQuests()
{
    for (QuestIdSet::const_iterator itr = classQuestIds.begin(); itr != classQuestIds.end(); ++itr)
    {
        uint32 questId = *itr;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (!bot->SatisfyQuestClass(quest, false))
        {
            LOG_TRACE("module", "[catchup] Bot not the right class for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (!bot->SatisfyQuestRace(quest, false))
        {
            LOG_TRACE("module", "[catchup] Bot not the right race for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (quest->GetMinLevel() > bot->GetLevel())
        {
            LOG_TRACE("module", "[catchup] Bot not high enough level for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        LOG_INFO("module", "[catchup] Bot needs to complete class quest: {} {}", questId, quest->GetTitle());
        requiredQuestIds.insert(questId);
    }
}

void CatchUpQuestManager::SatisfyPreQuests() { SatisfyPreQuests(requiredQuestIds); }

void CatchUpQuestManager::CompleteQuests()
{
    LOG_INFO("module", "[catchup] {} quests to complete", requiredQuestIds.size());
}