#include "ScriptPCH.h"
#include "Pet.h"

class npc_beastmaster : public CreatureScript
{
public:
        npc_beastmaster() : CreatureScript("npc_beastmaster") { }

    void CreatePet(Player *player, Creature * m_creature, uint32 entry)
    {
        if(player->getClass() != CLASS_HUNTER)
        {
            m_creature->MonsterWhisper("Вы не охотник!", player->GetGUIDLow());
            player->PlayerTalkClass->SendCloseGossip();
            return;
        }

        if(player->GetPet())
        {
            m_creature->MonsterWhisper("Вы должны отпусить своего питомца!", player->GetGUIDLow());
            player->PlayerTalkClass->SendCloseGossip();
            return;
        }

        Creature *creatureTarget = m_creature->SummonCreature(entry, player->GetPositionX(), player->GetPositionY()+2, player->GetPositionZ(), player->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 500);
        if(!creatureTarget) return;
        
        Pet* pet = player->CreateTamedPetFrom(creatureTarget, 0);
        if(!pet) return;

        // kill original creature
        creatureTarget->setDeathState(JUST_DIED);
        creatureTarget->RemoveCorpse();
        creatureTarget->SetHealth(0);                       // just for nice GM-mode view

        //pet->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE,0);
        //pet->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32((Trinity::XP::xp_to_level(70))/4));

        // prepare visual effect for levelup
        pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel() - 1);
        pet->GetMap()->AddToMap((Creature*)pet);
        // visual effect for levelup
        pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel());

        
        if(!pet->InitStatsForLevel(player->getLevel()))
            TC_LOG_ERROR("network", "Pet Create fail: no init stats for entry %u", entry);

        pet->UpdateAllStats();
        
        // caster have pet now
        player->SetMinion(pet, true);

        pet->SavePetToDB();
        pet->LearnPetPassives();
        pet->InitLevelupSpellsForLevel();
        player->PetSpellInitialize();
        
        //end
        player->PlayerTalkClass->SendCloseGossip();
        m_creature->MonsterWhisper("Питомец добавлен в стойло,теперь вы можете делать с ним все, что пожелаете.", player->GetGUIDLow());
    }


    bool OnGossipHello(Player *player, Creature * m_creature)
    {

        if(player->getClass() != CLASS_HUNTER)
        {
            m_creature->MonsterWhisper("Вы не охотник!", player->GetGUIDLow());
            return true;
        }
        player->ADD_GOSSIP_ITEM(4, "Получить нового питомца", GOSSIP_SENDER_MAIN, 30);

        if (player->CanTameExoticPets())
            player->ADD_GOSSIP_ITEM(4, "Получить нового экзотического питомца", GOSSIP_SENDER_MAIN, 50);

        player->ADD_GOSSIP_ITEM(2, "Заглянуть в стойло", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_STABLEPET);
        player->ADD_GOSSIP_ITEM(6, "Купить пищу для питомца", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_VENDOR);
        player->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 150);
        player->PlayerTalkClass->SendGossipMenu(20004, m_creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature * m_creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case 100:
                player->ADD_GOSSIP_ITEM(4, "Получить нового питомца", GOSSIP_SENDER_MAIN, 30);
                if (player->CanTameExoticPets())
                {
                    player->ADD_GOSSIP_ITEM(4, "Получить нового экзотического питомца", GOSSIP_SENDER_MAIN, 50);
                }
                player->ADD_GOSSIP_ITEM(2, "Заглянуть в стойло", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_STABLEPET);
                player->ADD_GOSSIP_ITEM(6, "Купить пищу для питомца", GOSSIP_SENDER_MAIN, GOSSIP_OPTION_VENDOR);
                player->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 150);
                player->PlayerTalkClass->SendGossipMenu(20004, m_creature->GetGUID());
                break;
            case 150:
                player->PlayerTalkClass->SendCloseGossip();
            break;
            case 30:
                player->ADD_GOSSIP_ITEM(2, "<- вернуться в основное меню", GOSSIP_SENDER_MAIN, 100);
                player->ADD_GOSSIP_ITEM(4, "Следующая страница ->", GOSSIP_SENDER_MAIN, 31);
                player->ADD_GOSSIP_ITEM(6, "Летучая мышь", GOSSIP_SENDER_MAIN, 18);
                player->ADD_GOSSIP_ITEM(6, "Медведь", GOSSIP_SENDER_MAIN, 1);
                player->ADD_GOSSIP_ITEM(6, "Кабан", GOSSIP_SENDER_MAIN, 2);
                player->ADD_GOSSIP_ITEM(6, "Кошка", GOSSIP_SENDER_MAIN, 4);
                player->ADD_GOSSIP_ITEM(6, "Падальщик", GOSSIP_SENDER_MAIN, 5);
                player->ADD_GOSSIP_ITEM(6, "Краб", GOSSIP_SENDER_MAIN, 6);
                player->ADD_GOSSIP_ITEM(6, "Кроколиск", GOSSIP_SENDER_MAIN, 7);
                player->ADD_GOSSIP_ITEM(6, "Дракондор", GOSSIP_SENDER_MAIN, 17);
                player->ADD_GOSSIP_ITEM(6, "Горилла", GOSSIP_SENDER_MAIN, 8);
                player->ADD_GOSSIP_ITEM(6, "Гончая", GOSSIP_SENDER_MAIN, 9);
                player->ADD_GOSSIP_ITEM(6, "Гиена", GOSSIP_SENDER_MAIN, 10);
                player->ADD_GOSSIP_ITEM(6, "Мотылек", GOSSIP_SENDER_MAIN, 11);
                player->ADD_GOSSIP_ITEM(6, "Сова", GOSSIP_SENDER_MAIN, 12);
                player->SEND_GOSSIP_MENU(1, m_creature->GetGUID());
                break;
            case 31:  
                player->ADD_GOSSIP_ITEM(2, "<- вернуться в основное меню", GOSSIP_SENDER_MAIN, 30);
                player->ADD_GOSSIP_ITEM(4, "<- предыдущая страница", GOSSIP_SENDER_MAIN, 30);
                player->ADD_GOSSIP_ITEM(6, "Раптор", GOSSIP_SENDER_MAIN, 20);
                player->ADD_GOSSIP_ITEM(6, "Опустошитель", GOSSIP_SENDER_MAIN, 19);
                player->ADD_GOSSIP_ITEM(6, "", GOSSIP_SENDER_MAIN, 13);
                player->ADD_GOSSIP_ITEM(6, "Скорпид", GOSSIP_SENDER_MAIN, 414);
                player->ADD_GOSSIP_ITEM(6, "Страйдер", GOSSIP_SENDER_MAIN, 16);
                player->ADD_GOSSIP_ITEM(6, "Змея", GOSSIP_SENDER_MAIN, 21);  
                player->ADD_GOSSIP_ITEM(6, "Черепаха", GOSSIP_SENDER_MAIN, 15);
                player->ADD_GOSSIP_ITEM(6, "Оса", GOSSIP_SENDER_MAIN, 93);
                player->SEND_GOSSIP_MENU(1, m_creature->GetGUID());
                break;
            case 50:  
                player->ADD_GOSSIP_ITEM(2, "<- вернуться в основное меню", GOSSIP_SENDER_MAIN, 100);
                player->ADD_GOSSIP_ITEM(2, "<- Получить нового нормального питомца", GOSSIP_SENDER_MAIN, 100);
                player->ADD_GOSSIP_ITEM(6, "Химера", GOSSIP_SENDER_MAIN, 51);
                player->ADD_GOSSIP_ITEM(6, "Гончая недр", GOSSIP_SENDER_MAIN, 52);
                player->ADD_GOSSIP_ITEM(6, "Дьявозавр", GOSSIP_SENDER_MAIN, 53);
                player->ADD_GOSSIP_ITEM(6, "Люторог", GOSSIP_SENDER_MAIN, 54);
                player->ADD_GOSSIP_ITEM(6, "Силитид", GOSSIP_SENDER_MAIN, 55);
                player->ADD_GOSSIP_ITEM(6, "Червь", GOSSIP_SENDER_MAIN, 56);  
                player->ADD_GOSSIP_ITEM(6, "Локе'нахак", GOSSIP_SENDER_MAIN, 57);
                player->ADD_GOSSIP_ITEM(6, "Сколл", GOSSIP_SENDER_MAIN, 58);
                player->ADD_GOSSIP_ITEM(6, "Гондрия", GOSSIP_SENDER_MAIN, 59);
                player->ADD_GOSSIP_ITEM(6, "Арктур", GOSSIP_SENDER_MAIN, 60);
                player->SEND_GOSSIP_MENU(1, m_creature->GetGUID());
                break;
            case GOSSIP_OPTION_STABLEPET:
                player->GetSession()->SendStablePet(m_creature->GetGUID());
                break; 
            case GOSSIP_OPTION_VENDOR:
                player->GetSession()->SendListInventory(m_creature->GetGUID());
                break;
            case 51: //chimera
                CreatePet(player, m_creature, 21879);
                break;
            case 52: //core hound
                CreatePet(player, m_creature, 21108);
                break;
            case 53: //Devilsaur
                CreatePet(player, m_creature, 20931);
                break;
            case 54: //rhino
                CreatePet(player, m_creature, 30445);
                break;
            case 55: //silithid
                CreatePet(player, m_creature, 5460);
                break;
            case 56: //Worm
                CreatePet(player, m_creature, 30148);
                break;
            case 57: //Loque'nahak
                CreatePet(player, m_creature, 32517);
                break;
            case 58: //Skoll
                CreatePet(player, m_creature, 35189);
                break;
            case 59: //Gondria
                CreatePet(player, m_creature, 33776);
                break;
            case 60: //Arcturis
                CreatePet(player, m_creature, 38453);
                break;
            case 16: //Spider
                CreatePet(player, m_creature, 2349);
                break;
            case 17: //Dragonhawk
                CreatePet(player, m_creature, 27946);
                break;
            case 18: //Bat
                CreatePet(player, m_creature, 28233);
                break;
            case 19: //Ravager
                CreatePet(player, m_creature, 17199);
                break;
            case 20: //Raptor
                CreatePet(player, m_creature, 14821);
                break;
            case 21: //Serpent
                CreatePet(player, m_creature, 28358);
                break;
            case 1: //bear
                CreatePet(player, m_creature, 29319);
                break;
            case 2: //Boar
                CreatePet(player, m_creature, 29996);
                break;
            case 93: //Bug
                CreatePet(player, m_creature, 28085);
                break;
            case 4: //cat
                CreatePet(player, m_creature, 28097);
                break;
            case 5: //carrion
                CreatePet(player, m_creature, 26838);
                break;
            case 6: //crab
                CreatePet(player, m_creature, 24478);
                break;   
            case 7: //crocolisk
                CreatePet(player, m_creature, 1417);
                break;  
            case 8: //gorila
                CreatePet(player, m_creature, 28213);
                break;
            case 9: //hound
                CreatePet(player, m_creature, 29452);
                break;
            case 10: //hynea
                CreatePet(player, m_creature, 13036);
                break;
            case 11: //Moth
                CreatePet(player, m_creature, 27421);
                break;
            case 12: //owl
                CreatePet(player, m_creature, 23136);
                break;
            case 13: //strider
                CreatePet(player, m_creature, 22807);
                break;
            case 414: //scorpid
                CreatePet(player, m_creature, 9698);
                break;
            case 15: //turtle
                CreatePet(player, m_creature, 25482);
                break;
            default:
                break;
        }
        
        return true;
    }
};

void AddSC_npc_beastmaster()
{
    new npc_beastmaster();
}