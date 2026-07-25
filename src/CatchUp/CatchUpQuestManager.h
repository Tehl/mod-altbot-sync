#ifndef ALTBOT_SYNC_CATCHUPQUESTMANAGER_H
#define ALTBOT_SYNC_CATCHUPQUESTMANAGER_H

#include "Player.h"

typedef std::unordered_set<uint32> QuestIdSet;

class CatchUpQuestManager
{
public:
    static void Init();

    CatchUpQuestManager(Player* bot);

    void AddPlayerQuests(Player* player);
    void AddClassQuests();
    void SatisfyPreQuests();
    void CompleteQuests();

private:
    static void SatisfyPreQuests(QuestIdSet& questSet);

    static QuestIdSet classQuestIds;

    Player* bot;
    QuestIdSet requiredQuestIds;
};

#endif