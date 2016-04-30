#include "ScriptPCH.h"

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
         npc_custom_starterAI(Creature* creature) : ScriptedAI(creature) {}    
            
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
             _introDone = false;
             _start_event = false;
             phase = 0;
             timer = 0;
             _two_event = false;
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Ты нас понимаешь?", LANG_UNIVERSAL);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;
                case 1:
                    if (timer < diff) {
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Yell("МЯУ!", LANG_UNIVERSAL);
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Прости нас, пожалуйста, мы не хотели!", LANG_UNIVERSAL);
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
                        if (Player* player = me->SelectNearestPlayer(80.0f))
                           player->Say("Послушай, а где мы?", LANG_UNIVERSAL);
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Пленники? Но чьи?", LANG_UNIVERSAL);
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Алиса...? Это кто еще?", LANG_UNIVERSAL);
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
                                 rand = urand (1, 8);
                                 if (rand == 1)
                                    plr->Yell("Как нам быть?", LANG_UNIVERSAL);
                                 if (rand == 2)
                                    plr->Yell("ЧТО!?", LANG_UNIVERSAL);
                                 if (rand == 3)
                                    plr->Yell("МЫ ВСЕ УМРЁМ!", LANG_UNIVERSAL);
                                 if (rand == 4)
                                    plr->Yell("Мы даже понятия не имеем где мы, а вдруг эта мышь водит нас за нос в своих целях? Лучше доверять человеку, чем вредителю!", LANG_UNIVERSAL);
                                 if (rand == 5)
                                    plr->Yell("Приключения!!!", LANG_UNIVERSAL);
                                 if (rand == 6)
                                    plr->Yell("Что вообще происходит? Я слакал.", LANG_UNIVERSAL);
                                 if (rand == 7)
                                    plr->Yell("А лут точно будет?", LANG_UNIVERSAL);
                                 if (rand == 8)
                                    plr->Yell("УРААА!", LANG_UNIVERSAL);     
                              }                        
                           }
                       }
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Мужики... а... тут... крыса говорящая...", LANG_UNIVERSAL);                       
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Растения... Отобрали? Ты не шутишь?", LANG_UNIVERSAL);
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
                        if (Player* player = me->SelectNearestPlayer(20.0f))
                           player->Say("Прямо сказка какая-то, мышь говорит, растения живые...", LANG_UNIVERSAL);
                        phase++;
                        timer = 5000;
                    }
                    else timer -= diff;  
                    break;                 
                case 16:
                    if (timer < diff) {
                     std::list<Player*> targets;
                     GetPlayerListInGrid(targets, me, 40);
                     Trinity::Containers::RandomResizeList(targets, 2);
                     for (std::list<Player*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                        if (!targets.empty())
                                 (*itr)->Say("Спокойно! Не важно, Рилари, веди нас, мы поможем тебе. Надеюсь ты нас не подведешь.", LANG_UNIVERSAL);
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