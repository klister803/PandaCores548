#include "ScriptPCH.h"
#include "Language.h"

class npc_profession : public CreatureScript
{
        public:
        npc_profession () : CreatureScript("npc_profession") {}
                
                void CreatureWhisperBasedOnBool(const char *text, Creature *_creature, Player *pPlayer, bool value)
                {
                        if (value)
                                _creature->MonsterWhisper(text, pPlayer->GetGUIDLow());
                }

                uint32 PlayerMaxLevel() const
                {
                        return sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
                }

                bool PlayerHasItemOrSpell(const Player *plr, uint32 itemId, uint32 spellId) const
                {
                        return plr->HasItemCount(itemId, 1, true) || plr->HasSpell(spellId);
                }

                bool OnGossipHello(Player *pPlayer, Creature* _creature)
                {
                        pPlayer->ADD_GOSSIP_ITEM(9, "Список профессий", GOSSIP_SENDER_MAIN, 196);
                        pPlayer->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 220);
                        pPlayer->PlayerTalkClass->SendGossipMenu(20011, _creature->GetGUID());
                        return true;
                }
                
                bool PlayerAlreadyHasTwoProfessions(const Player *pPlayer) const
                {
                        uint32 skillCount = 0;

                        if (pPlayer->HasSkill(SKILL_MINING))
                                skillCount++;
                        if (pPlayer->HasSkill(SKILL_SKINNING))
                                skillCount++;
                        if (pPlayer->HasSkill(SKILL_HERBALISM))
                                skillCount++;

                        if (skillCount >= 2)
                                return true;

                        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
                        {
                                SkillLineEntry const *SkillInfo = sSkillLineStore.LookupEntry(i);
                                if (!SkillInfo)
                                        continue;

                                if (SkillInfo->categoryId == SKILL_CATEGORY_SECONDARY)
                                        continue;

                                if ((SkillInfo->categoryId != SKILL_CATEGORY_PROFESSION) || !SkillInfo->canLink)
                                        continue;

                                const uint32 skillID = SkillInfo->id;
                                if (pPlayer->HasSkill(skillID))
                                        skillCount++;

                                if (skillCount >= 2)
                                        return true;
                        }
                        
                        return false;
                }

                bool LearnAllRecipesInProfession(Player *pPlayer, SkillType skill)
                {
                        ChatHandler handler(pPlayer->GetSession());
                        char* skill_name;

                        SkillLineEntry const *SkillInfo = sSkillLineStore.LookupEntry(skill);
                        skill_name = SkillInfo->name;

                        if (!SkillInfo)
                        {
                                //TC_LOG_ERROR("Profession NPC: received non-valid skill ID (LearnAllRecipesInProfession)");
                                return false;
                        }       

                        LearnSkillRecipesHelper(pPlayer, SkillInfo->id);

                        pPlayer->SetSkill(SkillInfo->id, pPlayer->GetSkillStep(SkillInfo->id), 600, 600);
                        handler.PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, skill_name);                                    
                        return true;
                }
        
                void LearnSkillRecipesHelper(Player *player, uint32 skill_id)
                {
                        uint32 classmask = player->getClassMask();

                        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
                        {
                                SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
                                if (!skillLine)
                                        continue;

                                // wrong skill
                                if (skillLine->skillId != skill_id)
                                        continue;

                                // not high rank
                                if (skillLine->forward_spellid)
                                        continue;

                                // skip racial skills
                                if (skillLine->racemask != 0)
                                        continue;

                                // skip wrong class skills
                                if (skillLine->classmask && (skillLine->classmask & classmask) == 0)
                                        continue;

                                SpellInfo const * spellInfo = sSpellMgr->GetSpellInfo(skillLine->spellId);
                                if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, player, false))
                                        continue;
                                
                                player->learnSpell(skillLine->spellId, false);
                        }
                }

                bool IsSecondarySkill(SkillType skill) const
                {
                        return skill == SKILL_COOKING || skill == SKILL_FIRST_AID;
                }

                void CompleteLearnProfession(Player *pPlayer, Creature *pCreature, SkillType skill)
                {
                        if (PlayerAlreadyHasTwoProfessions(pPlayer) && !IsSecondarySkill(skill))
                                pCreature->MonsterWhisper("Вы уже изучили две профессии!", pPlayer->GetGUIDLow());
                        else
                        {
                                if (!LearnAllRecipesInProfession(pPlayer, skill))
                                        pCreature->MonsterWhisper("Внутренняя ошибка!", pPlayer->GetGUIDLow());
                        }
                }
        
                bool OnGossipSelect(Player* pPlayer, Creature* _creature, uint32 /*uiSender*/, uint32 uiAction)
                {                      
                                pPlayer->PlayerTalkClass->ClearMenus();
                                
                                switch (uiAction)
                                {
                                        case 200:
                                        pPlayer->ADD_GOSSIP_ITEM(9, "Список профессий", GOSSIP_SENDER_MAIN, 196);
                                        pPlayer->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 220);
                                        pPlayer->PlayerTalkClass->SendGossipMenu(20011, _creature->GetGUID());                            
                                        break;
                                        
                                        case 220:
                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                        break;
                                        
                                        case 196:                                                
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Алхимия", GOSSIP_SENDER_MAIN, 1);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Кузнечное дело", GOSSIP_SENDER_MAIN, 2);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Кожевничество", GOSSIP_SENDER_MAIN, 3);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Портняжное дело", GOSSIP_SENDER_MAIN, 4);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Инженерное дело", GOSSIP_SENDER_MAIN, 5);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Наложение чар", GOSSIP_SENDER_MAIN, 6);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Ювелирное дело", GOSSIP_SENDER_MAIN, 7);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Начертание", GOSSIP_SENDER_MAIN, 8);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Кулинария", GOSSIP_SENDER_MAIN, 9);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Первая помощь", GOSSIP_SENDER_MAIN, 10);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Травничество", GOSSIP_SENDER_MAIN, 11);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Снятие шкур", GOSSIP_SENDER_MAIN, 12);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Горное дело", GOSSIP_SENDER_MAIN, 13);
                                                pPlayer->ADD_GOSSIP_ITEM(4, "Вернуться в главное меню!", GOSSIP_SENDER_MAIN, 200);
                                                pPlayer->PlayerTalkClass->SendGossipMenu(20011, _creature->GetGUID());                                                
                                                break;
                                        case 1:
                                                if(pPlayer->HasSkill(SKILL_ALCHEMY))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }

                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ALCHEMY);                                                
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 2:
                                                if(pPlayer->HasSkill(SKILL_BLACKSMITHING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_BLACKSMITHING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 3:
                                                if(pPlayer->HasSkill(SKILL_LEATHERWORKING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_LEATHERWORKING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 4:
                                                if(pPlayer->HasSkill(SKILL_TAILORING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_TAILORING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 5:
                                                if(pPlayer->HasSkill(SKILL_ENGINEERING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ENGINEERING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 6:
                                                if(pPlayer->HasSkill(SKILL_ENCHANTING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ENCHANTING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 7:
                                                if(pPlayer->HasSkill(SKILL_JEWELCRAFTING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_JEWELCRAFTING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 8:
                                                if(pPlayer->HasSkill(SKILL_INSCRIPTION))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_INSCRIPTION);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 9:
                                                if(pPlayer->HasSkill(SKILL_COOKING))
                                                {    
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_COOKING);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 10:
                                                if(pPlayer->HasSkill(SKILL_FIRST_AID))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_FIRST_AID);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 11:
                                            if(pPlayer->HasSkill(SKILL_HERBALISM))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_HERBALISM);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                        case 12:
                                            if(pPlayer->HasSkill(SKILL_SKINNING))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_SKINNING);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                        case 13:
                                            if(pPlayer->HasSkill(SKILL_MINING))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_MINING);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                }
                    
                    return true;
                }
};

class npc_profession_for_donate: public CreatureScript
{
        public:
        npc_profession_for_donate () : CreatureScript("npc_profession_for_donate") {}
                
                void CreatureWhisperBasedOnBool(const char *text, Creature *_creature, Player *pPlayer, bool value)
                {
                        if (value)
                                _creature->MonsterWhisper(text, pPlayer->GetGUIDLow());
                }

                uint32 PlayerMaxLevel() const
                {
                        return sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
                }

                bool PlayerHasItemOrSpell(const Player *plr, uint32 itemId, uint32 spellId) const
                {
                        return plr->HasItemCount(itemId, 1, true) || plr->HasSpell(spellId);
                }

                bool OnGossipHello(Player *pPlayer, Creature* _creature)
                {
                        pPlayer->ADD_GOSSIP_ITEM(9, "Хорошо, я понял. Покажите мне список профессий для покупки!", GOSSIP_SENDER_MAIN, 196);
                        pPlayer->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 220);
                        pPlayer->PlayerTalkClass->SendGossipMenu(100002, _creature->GetGUID());
                        return true;
                }
                
                bool PlayerAlreadyHasTwoProfessions(const Player *pPlayer) const
                {
                        uint32 skillCount = 0;

                        if (pPlayer->HasSkill(SKILL_MINING))
                                skillCount++;
                        if (pPlayer->HasSkill(SKILL_SKINNING))
                                skillCount++;
                        if (pPlayer->HasSkill(SKILL_HERBALISM))
                                skillCount++;

                        if (skillCount >= 2)
                                return true;

                        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
                        {
                                SkillLineEntry const *SkillInfo = sSkillLineStore.LookupEntry(i);
                                if (!SkillInfo)
                                        continue;

                                if (SkillInfo->categoryId == SKILL_CATEGORY_SECONDARY)
                                        continue;

                                if ((SkillInfo->categoryId != SKILL_CATEGORY_PROFESSION) || !SkillInfo->canLink)
                                        continue;

                                const uint32 skillID = SkillInfo->id;
                                if (pPlayer->HasSkill(skillID))
                                        skillCount++;

                                if (skillCount >= 2)
                                        return true;
                        }
                        
                        return false;
                }

                bool LearnAllRecipesInProfession(Player *pPlayer, SkillType skill)
                {
                        ChatHandler handler(pPlayer->GetSession());
                        char* skill_name;

                        SkillLineEntry const *SkillInfo = sSkillLineStore.LookupEntry(skill);
                        skill_name = SkillInfo->name;

                        if (!SkillInfo)
                        {
                                //TC_LOG_ERROR("Profession NPC: received non-valid skill ID (LearnAllRecipesInProfession)");
                                return false;
                        }       

                        LearnSkillRecipesHelper(pPlayer, SkillInfo->id);

                        pPlayer->SetSkill(SkillInfo->id, pPlayer->GetSkillStep(SkillInfo->id), 600, 600);
                        handler.PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, skill_name);                                    
                        return true;
                }
        
                void LearnSkillRecipesHelper(Player *player, uint32 skill_id)
                {
                        uint32 classmask = player->getClassMask();

                        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
                        {
                                SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
                                if (!skillLine)
                                        continue;

                                // wrong skill
                                if (skillLine->skillId != skill_id)
                                        continue;

                                // not high rank
                                if (skillLine->forward_spellid)
                                        continue;

                                // skip racial skills
                                if (skillLine->racemask != 0)
                                        continue;

                                // skip wrong class skills
                                if (skillLine->classmask && (skillLine->classmask & classmask) == 0)
                                        continue;

                                SpellInfo const * spellInfo = sSpellMgr->GetSpellInfo(skillLine->spellId);
                                if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, player, false))
                                        continue;
                                
                                player->learnSpell(skillLine->spellId, false);
                        }
                }

                bool IsSecondarySkill(SkillType skill) const
                {
                        return skill == SKILL_COOKING || skill == SKILL_FIRST_AID;
                }

                void CompleteLearnProfession(Player *pPlayer, Creature *pCreature, SkillType skill, uint32 cost)
                {
                        if (PlayerAlreadyHasTwoProfessions(pPlayer) && !IsSecondarySkill(skill))
                                pCreature->MonsterWhisper("Вы уже изучили две профессии!", pPlayer->GetGUIDLow());
                        else
                        {
                              if (pPlayer->GetItemCount(38186) >= cost)
                              {
                                   if (LearnAllRecipesInProfession(pPlayer, skill))
                                   {
                                           pPlayer->DestroyItemCount(38186, cost, true); 
                                           pPlayer->m_Events.AddEvent(new DelayedPlayerKickEvent(pPlayer), pPlayer->m_Events.CalculateTime(5000));
                                           pCreature->MonsterWhisper("Вы приобрели указанную профессию, а теперь Вы будете кикнуты через 5 секунд для завершения покупки!", pPlayer->GetGUIDLow());
                                           pPlayer->SaveToDB();
                                   }
                                   else
                                           pCreature->MonsterWhisper("Внутренняя ошибка", pPlayer->GetGUIDLow());
                              }
                              else
                                 pCreature->MonsterWhisper("Недостаточно Эфириальных монет для покупки!", pPlayer->GetGUIDLow());
                                
                        }
                }
      
                bool OnGossipSelect(Player* pPlayer, Creature* _creature, uint32 /*uiSender*/, uint32 uiAction)
                {                      
                                pPlayer->PlayerTalkClass->ClearMenus();
                                
                                switch (uiAction)
                                {
                                        case 200:
                                        pPlayer->ADD_GOSSIP_ITEM(9, "Хорошо, я понял. Покажите мне список профессий для покупки!", GOSSIP_SENDER_MAIN, 196);
                                        pPlayer->ADD_GOSSIP_ITEM(7, "Прощайте", GOSSIP_SENDER_MAIN, 220);
                                        pPlayer->PlayerTalkClass->SendGossipMenu(100002, _creature->GetGUID());                            
                                        break;
                                        
                                        case 220:
                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                        break;
                                        
                                        case 196:                                                
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Alchemy:30|t Алхимия - 145 Эфириальная монета", GOSSIP_SENDER_MAIN, 1);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_BlackSmithing:30|t Кузнечное дело - 150 Эфириальная монета", GOSSIP_SENDER_MAIN, 2);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_LeatherWorking:30|t Кожевничество - 145 Эфириальная монета", GOSSIP_SENDER_MAIN, 3);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Tailoring:30|t Портняжное дело - 135 Эфириальная монета", GOSSIP_SENDER_MAIN, 4);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Engineering:30|t Инженерное дело - 150 Эфириальная монета", GOSSIP_SENDER_MAIN, 5);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Engraving:30|t Наложение чар - 140 Эфириальная монета", GOSSIP_SENDER_MAIN, 6);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\INV_Misc_Gem_01:30|t Ювелирное дело - 150 Эфириальная монета", GOSSIP_SENDER_MAIN, 7);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\INV_Inscription_Tradeskill01:30|t Начертание - 145 Эфириальная монета", GOSSIP_SENDER_MAIN, 8);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\INV_Misc_Food_17:30|t Кулинария - 110 Эфириальная монета", GOSSIP_SENDER_MAIN, 9);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Achievement_BG_winSOA_underXminutes:30|t Первая помощь - 90 Эфириальная монета", GOSSIP_SENDER_MAIN, 10);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Herbalism:30|t Травничество - 100 Эфириальная монета", GOSSIP_SENDER_MAIN, 11);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\INV_Chest_Leather_17B:30|t Снятие шкур - 100 Эфириальная монета", GOSSIP_SENDER_MAIN, 12);
                                                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "|TInterface\\icons\\Trade_Mining:30|t Горное дело - 100 Эфириальная монета", GOSSIP_SENDER_MAIN, 13);
                                                pPlayer->ADD_GOSSIP_ITEM(4, "Вернуться в главное меню!", GOSSIP_SENDER_MAIN, 200);
                                                pPlayer->PlayerTalkClass->SendGossipMenu(20011, _creature->GetGUID());                                                
                                                break;
                                        case 1:
                                                if(pPlayer->HasSkill(SKILL_ALCHEMY))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }

                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ALCHEMY, 145);                                                
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 2:
                                                if(pPlayer->HasSkill(SKILL_BLACKSMITHING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_BLACKSMITHING, 150);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 3:
                                                if(pPlayer->HasSkill(SKILL_LEATHERWORKING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_LEATHERWORKING, 145);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 4:
                                                if(pPlayer->HasSkill(SKILL_TAILORING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_TAILORING, 135);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 5:
                                                if(pPlayer->HasSkill(SKILL_ENGINEERING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ENGINEERING, 150);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 6:
                                                if(pPlayer->HasSkill(SKILL_ENCHANTING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_ENCHANTING, 140);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 7:
                                                if(pPlayer->HasSkill(SKILL_JEWELCRAFTING))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_JEWELCRAFTING, 150);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 8:
                                                if(pPlayer->HasSkill(SKILL_INSCRIPTION))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_INSCRIPTION, 145);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 9:
                                                if(pPlayer->HasSkill(SKILL_COOKING))
                                                {    
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_COOKING, 110);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 10:
                                                if(pPlayer->HasSkill(SKILL_FIRST_AID))
                                                {
                                                        _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                        pPlayer->PlayerTalkClass->SendCloseGossip();
                                                        break;
                                                }
                                                CompleteLearnProfession(pPlayer, _creature, SKILL_FIRST_AID, 90);
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                        case 11:
                                            if(pPlayer->HasSkill(SKILL_HERBALISM))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_HERBALISM, 100);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                        case 12:
                                            if(pPlayer->HasSkill(SKILL_SKINNING))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_SKINNING, 100);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                        case 13:
                                            if(pPlayer->HasSkill(SKILL_MINING))
                                            {
                                                _creature->MonsterWhisper("Вы уже освоили эту профессию!", pPlayer->GetGUIDLow());
                                                pPlayer->PlayerTalkClass->SendCloseGossip();
                                                break;
                                            }
                                            
                                            CompleteLearnProfession(pPlayer, _creature, SKILL_MINING, 100);
                                            pPlayer->PlayerTalkClass->SendCloseGossip();
                                            break;
                                }
                    
                    return true;
                }
                
class DelayedPlayerKickEvent : public BasicEvent
{
public:
    DelayedPlayerKickEvent(Player* player) : player(player) { }

    bool Execute(uint64, uint32) override
    {
        player->GetSession()->KickPlayer();
        return true;
    }

private:
    Player* player;
};
                
};


void AddSC_npc_profession()
{
    new npc_profession();
    new npc_profession_for_donate();
}