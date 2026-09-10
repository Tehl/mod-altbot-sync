#include "CatchUpQuestManagerDebug.h"

#include <sstream>
#include <string>
#include <unordered_map>

#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"

namespace
{
std::unordered_map<uint8, std::string> class_names = {
    {CLASS_WARRIOR, "Warrior"}, {CLASS_PALADIN, "Paladin"}, {CLASS_HUNTER, "Hunter"},
    {CLASS_ROGUE, "Rogue"},     {CLASS_PRIEST, "Priest"},   {CLASS_DEATH_KNIGHT, "Death Knight"},
    {CLASS_SHAMAN, "Shaman"},   {CLASS_MAGE, "Mage"},       {CLASS_WARLOCK, "Warlock"},
    {CLASS_DRUID, "Druid"},
};

bool MatchesClassRequirement(uint32 reqClass, uint8 classId)
{
    uint32 classMask = 1 << (classId - 1);
    return (reqClass & classMask) > 0;
}

std::string FormatQuality(uint32 quality)
{
    switch (quality)
    {
        case 1:
            return "normal";
        case 2:
            return "uncommon";
        case 3:
            return "rare";
        case 4:
            return "epic";
        case 5:
            return "legendary";
        default:
            return std::to_string(quality);
    }
}

std::string FormatClassList(uint32 reqClass)
{
    std::string classString;

    for (uint8 classId = CLASS_WARRIOR; classId < MAX_CLASSES; ++classId)
    {
        if (!class_names.contains(classId))
            continue;

        if (!MatchesClassRequirement(reqClass, classId))
            continue;

        if (classString.size())
            classString += ", ";

        classString += class_names[classId];
    }

    return classString;
}
}  // namespace

bool CatchUpQuestManagerDebug::IsUnused(Quest const* quest)
{
    return quest->GetTitle().find("<", 0) != std::string::npos || quest->GetTitle() == "DESTROY THIS QUEST!";
}

QuestRewardType CatchUpQuestManagerDebug::CategoriseReward(ItemTemplate const* item)
{
    if (item->Quality > 2)
    {
        if (item->ItemLevel >= 60)
            return QUEST_REWARD_ENDGAME;

        return QUEST_REWARD_HIGH_QUALITY;
    }
    return QUEST_REWARD_REGULAR_QUALITY;
}

QuestRewardType CatchUpQuestManagerDebug::GetQuestRewardType(Quest const* quest)
{
    QuestRewardType bestReward = QUEST_REWARD_NONE;

    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            QuestRewardType rewardQuality = CategoriseReward(item);
            if (rewardQuality > bestReward)
                bestReward = rewardQuality;
        }
    }

    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardChoiceItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            QuestRewardType rewardQuality = CategoriseReward(item);
            if (rewardQuality > bestReward)
                bestReward = rewardQuality;
        }
    }

    if (bestReward == QUEST_REWARD_ENDGAME &&
        MatchesClassRequirement(quest->GetRequiredClasses(), CLASS_DEATH_KNIGHT) && quest->GetMinLevel() < 60)
        bestReward = QUEST_REWARD_HIGH_QUALITY;

    return bestReward;
}

void CatchUpQuestManagerDebug::PrintQuestForLog(Quest const* quest)
{
    LOG_INFO("module", "[catchup] - {} [{} | {}] {}", quest->GetQuestId(), quest->GetMinLevel(), quest->GetQuestLevel(),
             quest->GetTitle());

    LOG_INFO("module", "[catchup] - - Classes: {}", FormatClassList(quest->GetRequiredClasses()));

    std::string preQuests;
    for (Quest::PrevQuests::const_iterator itr_j = quest->prevQuests.begin(); itr_j != quest->prevQuests.end(); ++itr_j)
    {
        uint32 prevId = abs(*itr_j);

        if (preQuests.size())
            preQuests += "," + std::to_string(prevId);
        else
            preQuests += std::to_string(prevId);
    }
    if (preQuests.size())
        LOG_INFO("module", "[catchup] - - Pre-quests: {}", preQuests);

    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            LOG_INFO("module", "[catchup] - - Item: {} [{} | {}] {}", itemId, FormatQuality(item->Quality),
                     item->ItemLevel, item->Name1);
        }
    }

    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardChoiceItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            LOG_INFO("module", "[catchup] - - Item: {} [{} | {}] {}", itemId, FormatQuality(item->Quality),
                     item->ItemLevel, item->Name1);
        }
    }
}

void CatchUpQuestManagerDebug::PrintQuestForDocs(Quest const* quest)
{
    std::ostringstream markdown;

    markdown << "| [" << quest->GetQuestId() << "](https://www.wowhead.com/classic/quest=" << quest->GetQuestId()
             << ")";
    markdown << " | " << quest->GetMinLevel() << " (" << quest->GetQuestLevel() << ")";
    markdown << " | " << quest->GetTitle();
    markdown << " | " << FormatClassList(quest->GetRequiredClasses());

    std::string preQuests;
    for (Quest::PrevQuests::const_iterator itr_j = quest->prevQuests.begin(); itr_j != quest->prevQuests.end(); ++itr_j)
    {
        uint32 prevId = abs(*itr_j);

        if (preQuests.size())
            preQuests += "," + std::to_string(prevId);
        else
            preQuests += std::to_string(prevId);
    }

    markdown << " | " << preQuests;

    bool hasPrintedReward = false;
    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            if (hasPrintedReward)
                markdown << " |\r\n| | | | | | ";
            else
                markdown << " | ";

            markdown << "[" << itemId << "](https://www.wowhead.com/classic/item=" << itemId << ")" << " ["
                     << FormatQuality(item->Quality) << "] [" << item->ItemLevel << "] " << item->Name1;

            hasPrintedReward = true;
        }
    }

    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardChoiceItemId[i])
        {
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

            if (hasPrintedReward)
                markdown << " |\r\n| | | | | | ";
            else
                markdown << " | ";

            markdown << "[" << itemId << "](https://www.wowhead.com/classic/item=" << itemId << ")" << " ["
                     << FormatQuality(item->Quality) << "] [" << item->ItemLevel << "] " << item->Name1;

            hasPrintedReward = true;
        }
    }

    if (!hasPrintedReward)
        markdown << " | ";

    markdown << " |";
    LOG_INFO("module", markdown.str());
}

void CatchUpQuestManagerDebug::PrintQuestSet(QuestIdSet& questSet, bool docs)
{
    std::vector<uint32> questList{};
    questList.reserve(questSet.size());
    questList.insert(questList.end(), questSet.begin(), questSet.end());

    std::sort(questList.begin(), questList.end(), CatchUpQuestManager::SortQuestsByLevelAndChain);

    std::ostringstream idList;

    for (uint32 questId : questList)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (docs)
            PrintQuestForDocs(quest);
        else
            PrintQuestForLog(quest);

        idList << "," << questId;
    }

    if (docs)
        LOG_INFO("module", "[catchup] Quest Ids: {}", idList.str());
}

void CatchUpQuestManagerDebug::DebugPrintClassQuests(bool docs)
{
    QuestIdSet allClassQuests;
    QuestIdSet unusedClassQuests;
    QuestIdSet highQualityRewards;
    QuestIdSet endgameRewards;

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

        allClassQuests.insert(questId);

        if (IsUnused(quest))
        {
            unusedClassQuests.insert(questId);
            continue;
        }

        QuestRewardType rewardType = GetQuestRewardType(quest);
        if (rewardType == QUEST_REWARD_ENDGAME)
            endgameRewards.insert(questId);
        else if (rewardType == QUEST_REWARD_HIGH_QUALITY)
            highQualityRewards.insert(questId);
    }

    LOG_INFO("module", "[catchup] ----- ALL CLASS QUESTS -----");
    PrintQuestSet(allClassQuests, false);

    LOG_INFO("module", "[catchup] ----- UNUSED CLASS QUESTS -----");
    PrintQuestSet(unusedClassQuests, docs);

    LOG_INFO("module", "[catchup] ----- HIGH QUALITY REWARDS -----");
    PrintQuestSet(highQualityRewards, docs);

    LOG_INFO("module", "[catchup] ----- ENDGAME REWARDS -----");
    PrintQuestSet(endgameRewards, docs);

    CatchUpQuestManager::SatisfyPreQuests(endgameRewards);
    LOG_INFO("module", "[catchup] ----- ENDGAME REWARDS WITH PREQUESTS -----");
    PrintQuestSet(endgameRewards, docs);
}