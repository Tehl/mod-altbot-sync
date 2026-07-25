#ifndef ALTBOT_SYNC_CATCHUPQUESTMANAGER_H
#define ALTBOT_SYNC_CATCHUPQUESTMANAGER_H

#include "Player.h"

typedef std::unordered_set<uint32> QuestIdSet;

enum CompleteQuestResult : uint8
{
    COMPLETE_ERR_OK = 0,
    COMPLETE_ERR_INVENTORY_FULL = 1,
};

class CatchUpQuestManager
{
public:
    static void Init();

    CatchUpQuestManager(Player* bot);

    void AddPlayerQuests(Player* player);
    void AddClassQuests();
    void SatisfyPreQuests();
    CompleteQuestResult CompleteQuests();

private:
    static void SatisfyPreQuests(QuestIdSet& questSet);
    static bool SortQuestsByLevelAndChain(uint32 questIdA, uint32 questIdB);

    static QuestIdSet classQuestIds;

    CompleteQuestResult CompleteQuest(uint32 questId);

    Player* bot;
    QuestIdSet requiredQuestIds;
};

#endif