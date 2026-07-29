#ifndef ALTBOT_SYNC_CATCHUPQUESTMANAGER_H
#define ALTBOT_SYNC_CATCHUPQUESTMANAGER_H

#include "Player.h"
#include "PlayerbotAI.h"

typedef std::unordered_set<uint32> QuestIdSet;

enum CompleteQuestResult : uint8
{
    QUEST_ERR_OK = 0,
    QUEST_ERR_INVENTORY_FULL = 1,
};

class CatchUpQuestManager
{
public:
    static void Init();

    CatchUpQuestManager(Player* bot, PlayerbotAI* botAI);

    void AddPlayerQuests(Player* player);
    void AddClassQuests();
    void SatisfyPreQuests();
    CompleteQuestResult CompleteQuests();

private:
    static void SatisfyPreQuests(QuestIdSet& questSet);
    static bool SortQuestsByLevelAndChain(uint32 questIdA, uint32 questIdB);

    static QuestIdSet classQuestIds;

    CompleteQuestResult CompleteQuest(uint32 questId);
    CompleteQuestResult CheckInventorySpace(Quest const* quest);
    CompleteQuestResult FulfilQuestObjectives(Quest const* quest);
    uint32 ChooseRewardItem(Quest const* quest);
    void HandleRewards(Quest const* quest, uint32 reward);

    Player* bot;
    PlayerbotAI* botAI;
    QuestIdSet requiredQuestIds;
};

#endif