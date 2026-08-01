#include "CatchUpCharacterManager.h"

#include "PlayerbotFactory.h"

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