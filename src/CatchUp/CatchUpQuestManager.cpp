#include "CatchUpQuestManager.h"

#include "AltbotSyncConfig.h"
#include "EquipAction.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "SpellMgr.h"
#include "StatsWeightCalculator.h"

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
                LOG_TRACE("module", "[catchup] Quest {} {} needs pre-quest: {} {}", questId, quest->GetTitle(), prevId,
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

bool CatchUpQuestManager::SortQuestsByLevelAndChain(uint32 questIdA, uint32 questIdB)
{
    Quest const* questA = sObjectMgr->GetQuestTemplate(questIdA);
    Quest const* questB = sObjectMgr->GetQuestTemplate(questIdB);

    if (std::find(questB->prevQuests.begin(), questB->prevQuests.end(), questIdA) != questB->prevQuests.end())
    {
        // A is a pre-quest of B
        return true;
    }

    if (std::find(questA->prevQuests.begin(), questA->prevQuests.end(), questIdB) != questA->prevQuests.end())
    {
        // B is a pre-quest of A
        return false;
    }

    if (questA->GetMinLevel() < questB->GetMinLevel())
    {
        return true;
    }
    else if (questB->GetMinLevel() < questA->GetMinLevel())
    {
        return false;
    }

    if (questA->GetQuestLevel() < questB->GetQuestLevel())
    {
        return true;
    }
    else if (questB->GetQuestLevel() < questA->GetQuestLevel())
    {
        return false;
    }

    return questIdA < questIdB;
}

CatchUpQuestManager::CatchUpQuestManager(PlayerbotAI* botAI) : AiObject(botAI) {}

void CatchUpQuestManager::AddPlayerQuests(Player* player)
{
    const RewardedQuestSet& playerQuests = player->getRewardedQuests();

    for (RewardedQuestSet::const_iterator itr = playerQuests.begin(); itr != playerQuests.end(); ++itr)
    {
        uint32 const questId = *itr;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (sAltbotSyncConfig.playerQuestIgnoreList.contains(questId))
        {
            LOG_TRACE("module", "[catchup] Player quest marked as ignored: {} {}", questId, quest->GetTitle());
            continue;
        }

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

        LOG_TRACE("module", "[catchup] Bot needs to complete player quest: {} {}", questId, quest->GetTitle());
        requiredQuestIds.insert(questId);
    }
}

void CatchUpQuestManager::AddClassQuests()
{
    for (QuestIdSet::const_iterator itr = classQuestIds.begin(); itr != classQuestIds.end(); ++itr)
    {
        uint32 questId = *itr;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);

        if (sAltbotSyncConfig.classQuestIgnoreList.contains(questId))
        {
            LOG_TRACE("module", "[catchup] Class quest marked as ignored: {} {}", questId, quest->GetTitle());
            continue;
        }

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

        uint32 minLevel = quest->GetMinLevel();
        if (auto delayLevel = sAltbotSyncConfig.classQuestDelayList.find(quest->GetQuestId());
            delayLevel != sAltbotSyncConfig.classQuestDelayList.end())
        {
            LOG_INFO("module", "[catchup] Delaying level requirement for quest: {} {} until level {}", questId,
                     quest->GetTitle(), delayLevel->second);
            minLevel = delayLevel->second;
        }

        if (minLevel > bot->GetLevel())
        {
            LOG_TRACE("module", "[catchup] Bot not high enough level for quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        if (bot->IsQuestRewarded(questId))
        {
            LOG_TRACE("module", "[catchup] Bot already completed quest: {} {}", questId, quest->GetTitle());
            continue;
        }

        LOG_TRACE("module", "[catchup] Bot needs to complete class quest: {} {}", questId, quest->GetTitle());
        requiredQuestIds.insert(questId);
    }
}

void CatchUpQuestManager::SatisfyPreQuests() { SatisfyPreQuests(requiredQuestIds); }

CompleteQuestResult CatchUpQuestManager::CompleteQuests()
{
    LOG_INFO("module", "[catchup] {} quests to complete", requiredQuestIds.size());

    std::vector<uint32> questsToComplete{};
    questsToComplete.reserve(requiredQuestIds.size());
    questsToComplete.insert(questsToComplete.end(), requiredQuestIds.begin(), requiredQuestIds.end());

    std::sort(questsToComplete.begin(), questsToComplete.end(), SortQuestsByLevelAndChain);

    for (uint32 questId : questsToComplete)
    {
        CompleteQuestResult res = CompleteQuest(questId);
        if (res != QUEST_ERR_OK)
            return res;
    }

    return QUEST_ERR_OK;
}

CompleteQuestResult CatchUpQuestManager::CompleteQuest(uint32 questId)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    LOG_INFO("module", "[catchup] Processing quest {} {}", questId, quest->GetTitle());

    CompleteQuestResult inventoryResult = CheckInventorySpace(quest);
    if (inventoryResult != QUEST_ERR_OK)
        return inventoryResult;

    CompleteQuestResult objectiveResult = FulfilQuestObjectives(quest);
    if (objectiveResult != QUEST_ERR_OK)
        return objectiveResult;

    uint8 const currentLevel = bot->GetLevel();
    uint32 const currentXP = bot->GetUInt32Value(PLAYER_XP);

    bot->SetQuestStatus(questId, QUEST_STATUS_COMPLETE);

    uint32 const reward = ChooseRewardItem(quest);
    bot->RewardQuest(quest, reward, bot, false);

    bot->GiveLevel(currentLevel);
    bot->SetUInt32Value(PLAYER_XP, currentXP);

    HandleRewards(quest, reward);

    return QUEST_ERR_OK;
}

CompleteQuestResult CatchUpQuestManager::CheckInventorySpace(Quest const* quest)
{
    uint8 requiredInventorySpace = 0;
    // need one inventory slot per reward item
    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (quest->RewardItemId[i])
            ++requiredInventorySpace;
    }
    // need one inventory slot total if there's a choice of reward item
    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        if (quest->RewardChoiceItemId[i])
        {
            ++requiredInventorySpace;
            break;
        }
    }
    // also need an inventory slot for each item required to _QUEST_ the quest
    for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        if (quest->RequiredItemId[i] && quest->RequiredItemCount[i])
            ++requiredInventorySpace;
    }

    if (bot->GetFreeInventorySpace() < requiredInventorySpace)
    {
        LOG_INFO("module", "[catchup] Bot needs at least {} free inventory slots to complete quest: {} {}",
                 requiredInventorySpace, quest->GetQuestId(), quest->GetTitle());
        return QUEST_ERR_INVENTORY_FULL;
    }

    return QUEST_ERR_OK;
}

CompleteQuestResult CatchUpQuestManager::FulfilQuestObjectives(Quest const* quest)
{
    // ref QuestAction::CompleteQuest

    // Add quest items for quests that require items
    for (uint8 x = 0; x < QUEST_ITEM_OBJECTIVES_COUNT; ++x)
    {
        uint32 id = quest->RequiredItemId[x];
        uint32 count = quest->RequiredItemCount[x];
        if (!id || !count)
            continue;

        ItemPosCountVec dest;
        uint8 msg = bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count);
        if (msg == EQUIP_ERR_OK)
        {
            Item* item = bot->StoreNewItem(dest, id, true);
            bot->SendNewItem(item, count, true, false);
        }
    }

    // If the quest requires reputation to complete
    if (uint32 repFaction = quest->GetRepObjectiveFaction())
    {
        uint32 repValue = quest->GetRepObjectiveValue();
        uint32 curRep = bot->GetReputationMgr().GetReputation(repFaction);
        if (curRep < repValue)
            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(repFaction))
                bot->GetReputationMgr().SetReputation(factionEntry, repValue);
    }

    // If the quest requires money
    int32 ReqOrRewMoney = quest->GetRewOrReqMoney();
    if (ReqOrRewMoney < 0)
        bot->ModifyMoney(-ReqOrRewMoney);

    return QUEST_ERR_OK;
}

uint32 CatchUpQuestManager::ChooseRewardItem(Quest const* quest)
{
    // ref NewRpgBaseAction::BestRewardIndex
    if (quest->GetRewChoiceItemsCount() <= 1)
        return 0;

    ItemUsage bestUsage = ITEM_USAGE_NONE;
    for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
        if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE)
            bestUsage = ITEM_USAGE_EQUIP;
        else if (usage == ITEM_USAGE_BAD_EQUIP && bestUsage != ITEM_USAGE_EQUIP)
            bestUsage = ITEM_USAGE_BAD_EQUIP;
    }

    switch (bestUsage)
    {
        case ITEM_USAGE_EQUIP:
        case ITEM_USAGE_BAD_EQUIP:
            return ChooseRewardItemToEquip(quest, bestUsage);

        default:
            return ChooseRewardItemToVendor(quest);
    }
}

uint32 CatchUpQuestManager::ChooseRewardItemToEquip(Quest const* quest, ItemUsage bestUsage)
{
    LOG_INFO("module", "[catchup] Choosing best reward to equip for quest {} {}", quest->GetQuestId(),
             quest->GetTitle());

    StatsWeightCalculator calc(bot);
    uint32 bestItemIdx = 0;
    float bestScore = 0;
    for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
        if (usage == bestUsage || usage == ITEM_USAGE_REPLACE)
        {
            float score = calc.CalculateItem(quest->RewardChoiceItemId[i]);
            LOG_INFO("module", "[catchup] Item {} {} has item score {}", quest->RewardChoiceItemId[i],
                     sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[i])->Name1, score);
            if (score > bestScore)
            {
                bestScore = score;
                bestItemIdx = i;
            }
        }
    }

    LOG_INFO("module", "[catchup] Best reward to equip is {} {} with score {}", quest->RewardChoiceItemId[bestItemIdx],
             sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[bestItemIdx])->Name1, bestScore);

    return bestItemIdx;
}

uint32 CatchUpQuestManager::ChooseRewardItemToVendor(Quest const* quest)
{
    LOG_INFO("module", "[catchup] Choosing best reward to vendor for quest {} {}", quest->GetQuestId(),
             quest->GetTitle());

    uint32 bestItemIdx = 0;
    float bestPrice = 0;
    for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[i]);
        LOG_INFO("module", "[catchup] Item {} {} has sell price {}c", quest->RewardChoiceItemId[i], item->Name1,
                 item->SellPrice);

        if (item->SellPrice > bestPrice)
        {
            bestPrice = item->SellPrice;
            bestItemIdx = i;
        }
    }

    LOG_INFO("module", "[catchup] Best reward to vendor is {} {} with sell price {}c",
             quest->RewardChoiceItemId[bestItemIdx],
             sObjectMgr->GetItemTemplate(quest->RewardChoiceItemId[bestItemIdx])->Name1, bestPrice);

    return bestItemIdx;
}

void CatchUpQuestManager::HandleRewards(Quest const* quest, uint32 reward)
{
    // check if the quest rewards items which need to be processed
    bool hasRewardItems = false;
    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (quest->RewardItemId[i])
        {
            hasRewardItems = true;
            break;
        }
    }

    for (uint32 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
    {
        if (quest->RewardChoiceItemId[i])
        {
            hasRewardItems = true;
            break;
        }
    }

    if (!hasRewardItems)
    {
        // no items granted, nothing to do
        return;
    }

    // equip upgrades from inventory
    uint32 equippedBefore[EQUIPMENT_SLOT_END - EQUIPMENT_SLOT_START] = {};
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* equippedItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        equippedBefore[i] = equippedItem ? equippedItem->GetEntry() : 0;
    }

    EquipUpgradeAction equipUpgrades(botAI);
    ItemIds items = equipUpgrades.SelectInventoryItemsToEquip();
    equipUpgrades.EquipItems(items);

    uint32 equippedAfter[EQUIPMENT_SLOT_END - EQUIPMENT_SLOT_START] = {};
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* equippedItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        equippedAfter[i] = equippedItem ? equippedItem->GetEntry() : 0;
    }

    // identify unused items to be discarded
    std::list<std::tuple<uint32, uint32>> itemsToDiscard;
    for (uint32 i = 0; i < quest->GetRewItemsCount(); ++i)
    {
        if (uint32 itemId = quest->RewardItemId[i])
            itemsToDiscard.push_back({itemId, quest->RewardItemIdCount[i]});
    }
    if (uint32 itemId = quest->RewardChoiceItemId[reward])
        itemsToDiscard.push_back({itemId, 1});

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (equippedBefore[i] == equippedAfter[i])
            // slot hasn't changed
            continue;

        uint32 oldItem = equippedBefore[i];
        uint32 newItem = equippedAfter[i];

        if (oldItem != 0)
        {
            bool wasEquippedToDifferentSlot = false;
            for (uint8 j = EQUIPMENT_SLOT_START; j < EQUIPMENT_SLOT_END; ++j)
                if (equippedAfter[j] == oldItem)
                {
                    wasEquippedToDifferentSlot = true;
                    break;
                }

            if (!wasEquippedToDifferentSlot)
            {
                // old item has been unequipped
                LOG_INFO("module", "[catchup] Item {} {} was unequipped and will be discarded", oldItem,
                         sObjectMgr->GetItemTemplate(oldItem)->Name1);
                itemsToDiscard.push_back({oldItem, 1});
            }
        }

        if (newItem != 0)
        {
            bool wasPreviousEquipment = false;
            for (uint8 j = EQUIPMENT_SLOT_START; j < EQUIPMENT_SLOT_END; ++j)
                if (equippedBefore[j] == newItem)
                {
                    wasPreviousEquipment = true;
                    break;
                }

            if (!wasPreviousEquipment)
            {
                // new item wasn't previously equipped, so must come from quest
                LOG_INFO("module", "[catchup] Item {} {} was equipped and will be retained", newItem,
                         sObjectMgr->GetItemTemplate(newItem)->Name1);
                itemsToDiscard.remove_if([newItem](std::tuple<uint32, uint32> x) { return std::get<0>(x) == newItem; });
            }
        }
    }

    // vendor discarded items
    for (std::tuple<uint32, uint32> discardItem : itemsToDiscard)
    {
        uint32 itemId = std::get<0>(discardItem);
        uint32 itemCount = std::get<1>(discardItem);
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

        int32 vendorPrice = int64(uint64(item->SellPrice) * uint64(itemCount));

        LOG_INFO("module", "[catchup] Discarding {}x {} {} for {}c", itemCount, itemId, item->Name1, vendorPrice);

        bot->ModifyMoney(vendorPrice);
        bot->DestroyItemCount(itemId, itemCount, true);
    }
}