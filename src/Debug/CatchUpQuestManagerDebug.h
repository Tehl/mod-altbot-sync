#ifndef ALTBOT_SYNC_DEBUG_CATCHUPQUESTMANAGER_H
#define ALTBOT_SYNC_DEBUG_CATCHUPQUESTMANAGER_H

#include "CatchUpQuestManager.h"
#include "QuestDef.h"
#include "SharedDefines.h"

enum QuestRewardType : uint8
{
    QUEST_REWARD_NONE = 0,
    QUEST_REWARD_REGULAR_QUALITY = 1,
    QUEST_REWARD_HIGH_QUALITY = 2,
    QUEST_REWARD_ENDGAME = 3
};

class CatchUpQuestManagerDebug
{
public:
    static void DebugPrintClassQuests(bool docs = false);

private:
    static bool IsUnused(Quest const* quest);
    static QuestRewardType CategoriseReward(ItemTemplate const* item);
    static QuestRewardType GetQuestRewardType(Quest const* quest);
    static void PrintQuestForLog(Quest const* quest);
    static void PrintQuestForDocs(Quest const* quest);
    static void PrintQuestSet(QuestIdSet& questSet, bool docs);
};

#endif