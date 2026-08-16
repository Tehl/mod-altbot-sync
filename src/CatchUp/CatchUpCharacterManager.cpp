#include "CatchUpCharacterManager.h"

#include "PlayerbotFactory.h"

std::unordered_map<uint32, uint32> CatchUpCharacterManager::bags_vanilla = {
    {6, 4238},    // Linen Bag
    {8, 4240},    // Woolen Bag
    {10, 4245},   // Small Silk Pack
    {12, 10050},  // Mageweave Bag
    {14, 14046},  // Runecloth Bag
    {16, 14155},  // Mooncloth Bag
    {18, 14156}   // Bottomless Bag
};

std::unordered_map<uint32, uint32> CatchUpCharacterManager::bags_tbc = {
    {16, 21841},  // Netherweave Bag
    {18, 21843},  // Imbued Netherweave Bag
    {20, 21876},  // Primal Mooncloth Bag
    {22, 38082},  // Gigantique Bag
};

std::unordered_map<uint32, uint32> CatchUpCharacterManager::bags_wotlk = {
    {20, 41599},  // Frostweave Bag
    {22, 41600},  // Glacial Bag
    {24, 51809}   // Portable Hole
};

CatchUpCharacterManager::CatchUpCharacterManager(Player* bot) : bot(bot) {}

void CatchUpCharacterManager::GiveLevel(Player* player)
{
    bot->GiveLevel(player->GetLevel());
    bot->SetUInt32Value(PLAYER_XP, 0);
}

void CatchUpCharacterManager::GiveAbilities()
{
    PlayerbotFactory factory(bot, bot->GetLevel());

    factory.InitSkills();
    factory.InitClassSpells();
    factory.InitAvailableSpells();
    factory.InitPet();
}

void CatchUpCharacterManager::GiveBags(Player* player)
{
    PlayerbotFactory factory(bot, bot->GetLevel());

    // ref PlayerbotFactory::InitBags
    for (uint8 targetBagSlot = INVENTORY_SLOT_BAG_START; targetBagSlot < INVENTORY_SLOT_BAG_END; ++targetBagSlot)
    {
        Item* playerBag = player->GetItemByPos(INVENTORY_SLOT_BAG_0, targetBagSlot);
        if (!playerBag)
        {
            LOG_INFO("module", "[catchup] Player has nothing equipped in bag slot {}", targetBagSlot);
            continue;
        }

        LOG_INFO("module", "[catchup] Player has {} {} equipped in bag slot {} with size {}",
                 playerBag->GetTemplate()->ItemId, playerBag->GetTemplate()->Name1, targetBagSlot,
                 playerBag->GetTemplate()->ContainerSlots);

        uint32 desiredBagSize = playerBag->GetTemplate()->ContainerSlots;
        uint32 desiredBagId = ChooseBagTypeBySize(desiredBagSize);

        if (desiredBagId == 0)
        {
            LOG_INFO("module", "[catchup] No suitable bags available at level {} for size {}", bot->GetLevel(),
                     desiredBagSize);
            continue;
        }

        LOG_INFO("module", "[catchup] Bag {} {} is available at level {} for size {}", desiredBagId,
                 sObjectMgr->GetItemTemplate(desiredBagId)->Name1, bot->GetLevel(), desiredBagSize);

        Item* botOldBag = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, targetBagSlot);
        if (botOldBag && (botOldBag->GetTemplate()->ItemId == desiredBagId ||
                          botOldBag->GetTemplate()->ContainerSlots > desiredBagSize))
        {
            LOG_INFO("module", "[catchup] Bot already has bag {} {} with size {}", botOldBag->GetTemplate()->ItemId,
                     botOldBag->GetTemplate()->Name1, botOldBag->GetTemplate()->ContainerSlots);
            continue;
        }

        ItemPosCountVec dest;
        if (bot->CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, dest, desiredBagId, 1) != EQUIP_ERR_OK)
        {
            LOG_INFO("module", "[catchup] Bot unable to store new item {} {}", desiredBagId,
                     sObjectMgr->GetItemTemplate(desiredBagId)->Name1);
            continue;
        }

        Item* botNewBag = bot->StoreNewItem(dest, desiredBagId, true);
        if (botNewBag == nullptr)
        {
            LOG_INFO("module", "[catchup] Bot failed to store new item {} {}", desiredBagId,
                     sObjectMgr->GetItemTemplate(desiredBagId)->Name1);
            continue;
        }

        uint8 storedItemBagSlot = botNewBag->GetBagSlot();
        uint8 storedItemSlot = botNewBag->GetSlot();

        LOG_INFO("module", "[catchup] New item {} {} was added to bag {} slot {}", desiredBagId,
                 sObjectMgr->GetItemTemplate(desiredBagId)->Name1, storedItemBagSlot, storedItemSlot);

        uint16 swapSourceLocation = ((storedItemBagSlot << 8) | storedItemSlot);
        uint16 swapTargetLocation = ((INVENTORY_SLOT_BAG_0 << 8) | targetBagSlot);
        bot->SwapItem(swapSourceLocation, swapTargetLocation);

        Item* itemAfterSwap = bot->GetItemByPos(storedItemBagSlot, storedItemSlot);
        if (itemAfterSwap)
        {
            LOG_INFO("module", "[catchup] Cleaning up item {} {} at bag {} slot {}",
                     itemAfterSwap->GetTemplate()->ItemId, itemAfterSwap->GetTemplate()->Name1, storedItemBagSlot,
                     storedItemSlot);

            bot->DestroyItem(storedItemBagSlot, storedItemSlot, true);
        }
        else
            LOG_INFO("module", "[catchup] Nothing to clean up at bag {} slot {}", storedItemBagSlot, storedItemSlot);
    }
}

uint32 CatchUpCharacterManager::ChooseBagTypeBySize(uint32 size)
{
    uint8 const botLevel = bot->GetLevel();

    if (botLevel > 70)
        if (auto res = bags_wotlk.find(size); res != bags_wotlk.end())
            return res->second;

    if (botLevel > 60)
        if (auto res = bags_tbc.find(size); res != bags_tbc.end())
            return res->second;

    if (auto res = bags_vanilla.find(size); res != bags_vanilla.end())
        return res->second;

    return 0;
}