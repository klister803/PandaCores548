#include "ScriptPCH.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellMgr.h"
#include "LFGMgr.h"
#include "Group.h"
#include "Player.h"
#include "CreatureTextMgr.h"

class npc_custom_starter : public CreatureScript  //250025
{
   public:
      npc_custom_starter() : CreatureScript("npc_custom_starter") { }
            
      bool OnGossipHello(Player* player, Creature* creature)
          {
               player->CLOSE_GOSSIP_MENU();
               CAST_AI(npc_custom_starter::npc_custom_starterAI, creature->AI())->_two_event = true;
               CAST_AI(npc_custom_starter::npc_custom_starterAI, creature->AI())->timer = 10;
               CAST_AI(npc_custom_starter::npc_custom_starterAI, creature->AI())->phase = 0;                     
                 
              player->SEND_GOSSIP_MENU(1, creature->GetGUID());
              return true;
          }             
      
     struct npc_custom_starterAI : public ScriptedAI 
      { 
         npc_custom_starterAI(Creature* creature) : ScriptedAI(creature)
         {
             _introDone = false;
             _start_event = false;
             _two_event = false;             
         }    
            
         bool _introDone, _start_event, _two_event;
         uint32 timer;
         uint32 phase;
         uint8 rand;
         uint8 check;

         void MoveInLineOfSight(Unit* who)
         {
             if (!_introDone && me->IsWithinDistInMap(who, 5.0f))
             {
                 _introDone = true;
                 _start_event = true;
                 timer = 5000;
                 phase = 0;
                 me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
             }
         }
          
         void Reset()
         {
             phase = 0;
             timer = 0;
             rand = 0;
             check = 0;
         }
      
         void UpdateAI(uint32 diff)
        {
            if (_start_event) 
            {
                switch (phase) {
                case 0:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 0, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;
                case 1:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 1, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 1000;
                    }
                    else timer -= diff;  
                    break;                
                case 2:
                    if (timer < diff) {
                        Talk(2);
                        phase++;
                        _start_event = false;
                        me->GetMotionMaster()->MoveJump(879.24f, 871.15f, 92.69f, 15, 15);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                    else timer -= diff;  
                    break;                
                }
            }
            if (_two_event) 
            {
                switch (phase) {
                case 0:
                    if (timer < diff) {
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 19, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 1:
                    if (timer < diff) {
                        Talk(3); // Ну, конечно!
                        me->GetMotionMaster()->MoveJump(864.09f, 844.47f, 52.0f, 15, 15);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                  
                case 2:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 4, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 3:
                    if (timer < diff) {
                        Talk(5);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 4:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 6, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 5:
                    if (timer < diff) {
                        Talk(7);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 6:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 8, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;               
                case 7:
                    if (timer < diff) {
                        Talk(9);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                 
                case 8:
                    if (timer < diff) {
                        Talk(10);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                 
                case 9:
                    if (timer < diff) {
                        Talk(11);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 10:
                    if (timer < diff) {
                    const Map::PlayerList &PlayerList = me->GetMap()->GetPlayers();
                    if (PlayerList.isEmpty())
                        return;
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                       {
                           if (Player* plr = i->getSource())
                           {  
                              check = urand (0, 2);  // много флуда иначе
                              if (check == 1)
                              {
                                 sCreatureTextMgr->SendChat(me, 20, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, plr);    
                              }                        
                           }
                       }
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 21, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);                       
                        phase++;
                        timer = 9000;
                    }
                    else timer -= diff;  
                    break;    
                    
                case 11:
                    if (timer < diff) {
                        Talk(12);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;               
                case 12:
                    if (timer < diff) {
                        Talk(13);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 13:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 22, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 14:
                    if (timer < diff) {
                        Talk(15);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                
                case 15:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 23, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                 
                case 16:
                    if (timer < diff) {
                        Player* player = SelectRandomPlayer(80.0f, false);
                           sCreatureTextMgr->SendChat(me, 24, 0,  CHAT_MSG_SAY, LANG_UNIVERSAL, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;
                case 17:
                    if (timer < diff) {
                        Talk(16);
                        me->GetMotionMaster()->MovePoint(0, 863.13f, 842.88f, 51.60f);
                        phase++;
                        timer = 6000;
                    }
                    else timer -= diff;  
                    break; 
                case 18:
                    if (timer < diff) {
                        Talk(17);
                        phase++;
                        timer = 3000;
                    }
                    else timer -= diff;  
                    break;                
                case 19:
                    if (timer < diff) {
                        if (GameObject* GO = me->FindNearestGameObject(185905, 50.0f))
                          GO->SetGoState(GO_STATE_ACTIVE);
                        me->GetMotionMaster()->MovePoint(0, 828.34f, 818.62f, 37.92f);
                        phase++;
                        timer = 8000;
                    }
                    else timer -= diff;  
                    break;                
                case 20:
                    if (timer < diff) {
                        me->GetMotionMaster()->MovePoint(0, 815.73f, 770.42f, 30.54f);
                        phase++;
                        timer = 8000;
                    }
                    else timer -= diff;  
                    break;                
                case 21:
                    if (timer < diff) {
                        Talk(18);
                        phase++;
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        _two_event = false;
                    }
                    else timer -= diff;  
                    break;  
                }
            }
        }


        Player* SelectRandomPlayer(float range = 0.0f, bool checkLoS = true)
        {
            Map* map = me->GetMap();

            Map::PlayerList const &PlayerList = map->GetPlayers();
            if (PlayerList.isEmpty())
                return NULL;

            std::list<Player*> temp;
            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                if ((me->IsWithinLOSInMap(i->getSource()) || !checkLoS) && me->getVictim() != i->getSource() &&
                    me->IsWithinDistInMap(i->getSource(), range) && i->getSource()->IsAlive())
                    temp.push_back(i->getSource());

            if (!temp.empty())
            {
                std::list<Player*>::const_iterator j = temp.begin();
                advance(j, rand32()%temp.size());
                return (*j);
            }
            return NULL;
        }        
        
        };            
      
      CreatureAI* GetAI(Creature* creature) const 
      { 
        return new npc_custom_starterAI (creature); 
      } 
};



void AddSC_custum_event()
{
   new npc_custom_starter();
}