/*
phase 67851 ind 2 - for quest 14474 
*/

#include "ScriptPCH.h"
#include "CreatureTextMgr.h"
#include "ScriptedEscortAI.h"

enum isle_quests
{
    QUEST_DONT_GO_INTO_LIGHT                     = 14239,
    QUEST_GOBLIN_ESCAPE_PODS                     = 14474,  // Goblin Escape Pods or 14001
    QUEST_MINER_TROUBLES                         = 14021,  // Miner Troubles
    QUEST_CAPTURE_UNKNOWN                        = 14031,  // Capturing the Unknown
    QUEST_WEED_WHACKER                           = 14236,  // Weed Whacker
};

enum isle_spells
{
    //Intro
    SPELL_SUMMON_DOC_ZAPNOZZLE                   = 69018,  // Don't Go Into The Light!: Summon Doc Zapnozzle
    SPELL_NEAR_DEATH                             = 69010,  // Near Death!
    SPELL_DOC_TO_CHAR                            = 69085,  // Don't Go Into The Light!: Force Cast from Doc to Character + proc 69086
    SPELL_INTRO_VISUAL                           = 69085,
    SPELL_INTRO_RES                              = 69022,
    SPELL_INVISIBLE_INRO_DUMMY                   = 76354,
    SPELL_SUMMON_FRIGTNED_MINER                  = 68059,
    SPELL_SUMMON_ORE_CART                        = 68064,
    SPELL_VISUAL_ORE_CART_CHAIN                  = 68122,
    SPELL_VISUAL_CART_TRANSFORM                  = 68065,
    SPELL_PHOTO_VISUAL_SCREEN_EFFECT             = 70649,  // Capturing The Unknown: Player's Screen Effect
    SPELL_PHOTO_VISUAL_BIND_SIGHT                = 70641,  // Capturing The Unknown: Player's Bind Sight
    SPELL_PHOTO_SNAPSHOT                         = 68281,  // KTC Snapflash
    SPELL_SHOOT                                  = 15620,
    SPELL_VISUAL_VILE_CAPTURE                    = 68295,
    SPELL_SUMMON_WW_CHANNEL_BUNNY                = 68216, // Weed Whacker: Summon Weed Whacker Channel Bunny
    SPELL_WEED_WHACKER                           = 68212, // Weed Whacker DMG aura
    SPELL_WEED_WHACKER_BUF                       = 68824, //Weed Whacker
};

enum isle_npc
{
    NPC_DOC_ZAPNNOZZLE                           = 36608,
    NPC_GIZMO                                    = 36600, // Geargrinder Gizmo
    NPC_FRIGHTENED_MINER                         = 35813, // Frightened Miner
    NPC_FOREMAN_DAMPWICK                         = 35769, // Foreman Dampwick
    NPC_ORE_CART                                 = 35814, // Miner Troubles Ore Cart 35814
    NPC_QUEST_MINE_TROUBLES_CREDIT               = 35816,
};

enum isle_go
{
    GO_KAJAMITE_ORE                             = 195622, //Kaja'mite Ore
};

enum isle_events
{
    EVENT_THE_VERY_BEGINING_1                      = 1,
    EVENT_THE_VERY_BEGINING_2                      = 2,
    EVENT_THE_VERY_BEGINING_3                      = 3,
    EVENT_THE_VERY_BEGINING_4                      = 4,
    EVENT_THE_VERY_BEGINING_5                      = 5,
    EVENT_THE_VERY_BEGINING_6                      = 6,
    EVENT_THE_VERY_BEGINING_7                      = 7,
    EVENT_THE_VERY_BEGINING_8                      = 8,
    EVENT_THE_VERY_BEGINING_9                      = 9,
    EVENT_THE_VERY_BEGINING_10                     = 10,
    EVENT_THE_VERY_BEGINING_11                     = 11,
    EVENT_THE_VERY_BEGINING_12                     = 12,
    EVENT_THE_VERY_BEGINING_13                     = 13,

    EVENT_GENERIC_1                                = 1,
    EVENT_GENERIC_2                                = 2,

    EVENT_POINT_MINE                               = 1000,
};

enum isle_text
{
    TEXT_GENERIC_0                                 = 0,
    TEXT_GENERIC_1                                 = 1,
    TEXT_GENERIC_2                                 = 2,
    TEXT_GENERIC_3                                 = 3,
    TEXT_GENERIC_4                                 = 4,

};

enum isle_emote
{
    EMOTE_FIND_MINE                                = 396,
    EMOTE_FIND_MINING                              = 233,
    EMOTE_SAFE_ORC                                 = 66,
};

enum gizmo_text
{
    TEXT_GIZMO_QUEST                             = 0,
};


class npc_gizmo : public CreatureScript
{
    public:
        npc_gizmo() : CreatureScript("npc_gizmo") { }

    struct npc_gizmoAI : public ScriptedAI
    {
        npc_gizmoAI(Creature* creature) : ScriptedAI(creature)
        {
            me->m_invisibilityDetect.AddFlag(INVISIBILITY_UNK7);
            me->m_invisibilityDetect.AddValue(INVISIBILITY_UNK7, 100000);
        }

        std::set<uint64> m_player_for_event;
        void Reset()
        {
            m_player_for_event.clear();
        }

        void OnStartQuest(Player* player, Quest const* quest)
        {
            if (!quest || quest->GetQuestId() != QUEST_GOBLIN_ESCAPE_PODS)
                return;

            sCreatureTextMgr->SendChat(me, TEXT_GIZMO_QUEST, player ? player->GetGUID(): 0);
        }

        // Remove from conteiner for posibility repeat it.
        // If plr disconect or not finish quest.
        void SetGUID(uint64 guid, int32 id)
        {
            m_player_for_event.erase(guid);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->HasAura(SPELL_NEAR_DEATH))
            {
                // always player. don't warry
                // waiting then plr finish movie.
                if (who->ToPlayer()->isWatchingMovie())
                    return;

                std::set<uint64>::iterator itr = m_player_for_event.find(who->GetGUID());
                if (itr == m_player_for_event.end())
                {
                    m_player_for_event.insert(who->GetGUID());
                    //me->CastSpell(537.135f, 3272.25f, 0.18f, SPELL_SUMMON_DOC_ZAPNOZZLE, true);
                    Position pos;
                    pos.Relocate(537.135f, 3272.25f, 0.18f, 2.46f);
                    if (TempSummon* summon = me->GetMap()->SummonCreature(NPC_DOC_ZAPNNOZZLE, pos))
                    {
                        summon->AddPlayerInPersonnalVisibilityList(who->GetGUID());
                        summon->AI()->SetGUID(who->GetGUID(), 0);
                        summon->AI()->SetGUID(me->GetGUID(), 1);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {

        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_gizmoAI(creature);
    }
};



enum intro_text
{
    TEXT_INTRO_1                                   = 0,
    TEXT_INTRO_2                                   = 1,
    TEXT_INTRO_3                                   = 2,
    TEXT_INTRO_4                                   = 3,
    TEXT_INTRO_5                                   = 4,
};

class npc_doc_zapnnozzle : public CreatureScript
{
    public:
        npc_doc_zapnnozzle() : CreatureScript("npc_doc_zapnnozzle") { }

    struct npc_doc_zapnnozzleAI : public ScriptedAI
    {
        npc_doc_zapnnozzleAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->m_invisibilityDetect.AddFlag(INVISIBILITY_UNK7);
            creature->m_invisibilityDetect.AddValue(INVISIBILITY_UNK7, 100000);
        }

        uint64 plrGUID;
        uint64 gizmoGUID;
        EventMap events;

        void Reset()
        {
            plrGUID = 0;
            gizmoGUID = 0;
            events.Reset();
        }

        void SetGUID(uint64 guid, int32 id)
        {
            switch(id)
            {
                case 0: plrGUID = guid; break;
                case 1:
                    gizmoGUID = guid;
                    events.ScheduleEvent(EVENT_THE_VERY_BEGINING_1, 1000);
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_THE_VERY_BEGINING_1:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_1, plrGUID);
                        events.ScheduleEvent(++eventId, 2000);
                        break;
                    case EVENT_THE_VERY_BEGINING_2:
                        events.ScheduleEvent(++eventId, 3000);
                        if (Creature* gizmo = Unit::GetCreature(*me, gizmoGUID))
                            sCreatureTextMgr->SendChat(gizmo, TEXT_INTRO_1, plrGUID);
                        break;
                    case EVENT_THE_VERY_BEGINING_3:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_2, plrGUID);
                        events.ScheduleEvent(++eventId, 3000);
                        break;
                    case EVENT_THE_VERY_BEGINING_4:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_VISUAL);
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_3, plrGUID);
                        events.ScheduleEvent(++eventId, 1000);
                        break;
                    case EVENT_THE_VERY_BEGINING_5:
                    case EVENT_THE_VERY_BEGINING_6:
                    case EVENT_THE_VERY_BEGINING_8:
                    case EVENT_THE_VERY_BEGINING_9:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_RES);
                        events.ScheduleEvent(++eventId, 2500);
                        break;
                    case EVENT_THE_VERY_BEGINING_7:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_RES);
                        events.ScheduleEvent(++eventId, 2000);
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_4, plrGUID);
                        break;
                    case EVENT_THE_VERY_BEGINING_10:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_5, plrGUID);
                        events.ScheduleEvent(++eventId, 2000);
                        break;
                    case EVENT_THE_VERY_BEGINING_11:
                        me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            target->RemoveAura(SPELL_NEAR_DEATH);
                        me->RemoveAura(SPELL_INVISIBLE_INRO_DUMMY);   
                        me->DespawnOrUnsummon(30000);
                        if (Creature* gizmo = Unit::GetCreature(*me, gizmoGUID))
                            gizmo->AI()->SetGUID(plrGUID, 0);
                        break;
                    default:
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_doc_zapnnozzleAI(creature);
    }
};

// NPC_FOREMAN_DAMPWICK                         = 35769, //Foreman Dampwick

enum foreman_text
{
    TEXT_FOREMAN_0          = 0,
    TEXT_FOREMAN_1          = 1,
};

class npc_foreman_dampwick : public CreatureScript
{
    public:
        npc_foreman_dampwick() : CreatureScript("npc_foreman_dampwick") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    { 
        player->CLOSE_GOSSIP_MENU();
        if (action == 1)
        {
            if (player->GetQuestStatus(QUEST_MINER_TROUBLES) != QUEST_STATUS_FAILED)
                return true;
            player->IncompleteQuest(QUEST_MINER_TROUBLES);
            creature->AI()->OnStartQuest(player, sObjectMgr->GetQuestTemplate(QUEST_MINER_TROUBLES));
        }
        return true;
    }

    struct npc_foreman_dampwickAI : public ScriptedAI
    {
        npc_foreman_dampwickAI(Creature* creature) : ScriptedAI(creature)
        {

        }

        void Reset()
        {
        }

        void OnStartQuest(Player* player, Quest const* quest)   
        {
            if (!quest || quest->GetQuestId() != QUEST_MINER_TROUBLES)
                return;

            Position pos;
            pos.Relocate(492.4184f, 2976.321f, 8.040207f);
            if (TempSummon* summon = player->GetMap()->SummonCreature(NPC_FRIGHTENED_MINER, pos, NULL, 0, player))
            {
                summon->AddPlayerInPersonnalVisibilityList(player->GetGUID());
                summon->AI()->SetGUID(player->GetGUID(), 0);
                sCreatureTextMgr->SendChat(me, TEXT_FOREMAN_0, player->GetGUID());
            }
        }

        void OnQuestReward(Player* player, Quest const* quest)
        {
            if (!quest || quest->GetQuestId() != QUEST_MINER_TROUBLES)
                return;

            sCreatureTextMgr->SendChat(me, TEXT_FOREMAN_1, player->GetGUID());
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_foreman_dampwickAI(creature);
    }
};

enum miner_text
{
    TEXT_MINER_0      = 0,
    TEXT_MINER_1      = 1,
    TEXT_MINER_2      = 2,
    TEXT_MINER_3      = 3,
    TEXT_MINER_4      = 4,
    TEXT_MINER_5      = 5,
};

class npc_frightened_miner : public CreatureScript
{
public:
    npc_frightened_miner() : CreatureScript("npc_frightened_miner") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_frightened_minerAI (creature);
    }

    struct npc_frightened_minerAI : public npc_escortAI
    {
        npc_frightened_minerAI(Creature* creature) : npc_escortAI(creature) {}

        uint64 plrGUID;
        uint64 cartGUID;
        uint64 mineGUID;
        EventMap events;
        uint32 wpMine;

        void Reset()
        {
            plrGUID = 0;
            cartGUID = 0;
            mineGUID = 0;
            wpMine = 0;
            events.Reset();
            
        }

        void SetGUID(uint64 guid, int32 id)
        {
            plrGUID = guid;
            Start(true, false, guid);
            DoCast(me, SPELL_SUMMON_ORE_CART);
        }

        void EnterEvadeMode()
        {
            npc_escortAI::EnterEvadeMode();
            if (Creature* cart = Unit::GetCreature(*me, cartGUID))
                me->CastSpell(cart, SPELL_VISUAL_ORE_CART_CHAIN, true);
        }

        void JustSummoned(Creature* summon)
        {
            summon->AddPlayerInPersonnalVisibilityList(plrGUID);
            summon->SetWalk(false);
            summon->SetSpeed(MOVE_RUN, 1.25f);
            summon->GetMotionMaster()->MoveFollow(me, 1.0f, 0);
            cartGUID = summon->GetGUID();
        }

        void JustDied(Unit* /*killer*/) 
        {
            if (Player* player = GetPlayerForEscort())
                player->FailQuest(QUEST_MINER_TROUBLES);
        }

        void seachMine()
        {
            if (GameObject* mine = me->FindNearestGameObject(GO_KAJAMITE_ORE, 20.0f))
            {
                mineGUID = mine->GetGUID();
                SetEscortPaused(true);

                events.Reset();

                //Position pos;
                //mine->GetNearPosition(pos, 1.0f, 0.0f);
                me->GetMotionMaster()->MovePoint(EVENT_POINT_MINE, mine->m_positionX, mine->m_positionY, mine->m_positionZ);
                me->HandleEmoteCommand(EMOTE_FIND_MINE);
            }else if (HasEscortState(STATE_ESCORT_PAUSED))
                SetEscortPaused(false);
        }
        
        void MovementInform(uint32 moveType, uint32 pointId)
        {
            if (pointId == EVENT_POINT_MINE)
            {
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_FIND_MINING);
                events.ScheduleEvent(EVENT_GENERIC_1, 10000);
                return;
            }
            if (HasEscortState(STATE_ESCORT_PAUSED))
                seachMine();

            npc_escortAI::MovementInform(moveType, pointId);
        }

        void WaypointReached(uint32 pointId)
        {            
            switch(pointId)
            {
                case 1:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_0, plrGUID);
                    if (Creature* cart = Unit::GetCreature(*me, cartGUID))
                        me->CastSpell(cart, SPELL_VISUAL_ORE_CART_CHAIN, true);
                    break;
                case 7:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_1, plrGUID);
                    break;
                case 10:
                case 14:
                case 17:
                case 22:
                    wpMine = pointId;
                    seachMine();
                    break;
                case 23:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_5, plrGUID);
                    if (Player* player = GetPlayerForEscort())
                        player->KilledMonsterCredit(NPC_QUEST_MINE_TROUBLES_CREDIT);
                    break;
                case 24:
                    me->SetWalk(false);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // emotes only when in vehicle.
                    case EVENT_GENERIC_1:
                    {
                        uint32 text = 0;
                        switch(wpMine)
                        {
                            case 10: text = TEXT_MINER_2; break;
                            case 14: text = TEXT_MINER_3; break;
                            case 17: text = TEXT_MINER_4; break;
                        }
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                        SetEscortPaused(false);
                        if (text)
                            sCreatureTextMgr->SendChat(me, text, plrGUID);
                        if (GameObject* go = ObjectAccessor::GetGameObject(*me, mineGUID))
                        {
                            go->SendCustomAnim(0);
                            go->SetLootState(GO_JUST_DEACTIVATED);
                        }
                        break;
                    }
                }
            }
        }
    };
};


/*
DELETE FROM `spell_scripts` WHERE id = 68279;
INSERT INTO `spell_scripts` (`id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`) VALUES 
('68279', '0', '0', '15', '70649', '2', '0', '0', '0', '0', '0'),
('68279', '0', '1', '15', '70641', '3', '0', '0', '0', '0', '0'),
('68279', '0', '2', '15', '68281', '3', '0', '0', '0', '0', '0');
*/


/*-- on hit 68279 add aura  70649 on player
-- and cast 70641 on creature target witch add this aura - bind sight
-- and start channel 68281 on target witch add this aura
*/
class spell_photo_capturing : public SpellScriptLoader
{
public:
    spell_photo_capturing() : SpellScriptLoader("spell_photo_capturing") { }

    class spell_photo_capturing_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_photo_capturing_SpellScript);

        void HandleScript(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            Unit* caster =  GetCaster();
            if (!caster)
                return;

            Unit* target = GetHitUnit();
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return;

            if (target->GetTypeId() == TYPEID_PLAYER &&
                target->ToPlayer()->GetQuestStatus(QUEST_CAPTURE_UNKNOWN) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_PHOTO_VISUAL_SCREEN_EFFECT, true);
            target->CastSpell(caster, SPELL_PHOTO_VISUAL_BIND_SIGHT, true);
            target->CastSpell(caster, SPELL_PHOTO_SNAPSHOT, true);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_photo_capturing_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_photo_capturing_SpellScript();
    }
};

//Capturing The Unknown: KTC Snapflash Effect
class spell_ctu_snap_effect : public SpellScriptLoader
{
public:
    spell_ctu_snap_effect() : SpellScriptLoader("spell_ctu_snap_effect") { }

    class spell_ctu_snap_effect_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ctu_snap_effect_SpellScript);

        void HandleScript(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            Unit* caster =  GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            if (caster->GetTypeId() == TYPEID_PLAYER &&
                caster->ToPlayer()->GetQuestStatus(QUEST_CAPTURE_UNKNOWN) != QUEST_STATUS_INCOMPLETE)
                return;

            Unit* target = GetHitUnit();
            if (!target)
                return;

            for (int32 i = INVISIBILITY_UNK5; i < INVISIBILITY_UNK10; ++i)
            {
                if (target->m_invisibility.HasFlag((InvisibilityType)i))
                {
                    caster->m_invisibilityDetect.DelFlag((InvisibilityType)i);
                    break;
                }
            }
            caster->UpdateObjectVisibility();
            caster->RemoveAurasDueToSpell(SPELL_PHOTO_VISUAL_SCREEN_EFFECT);
            caster->ToPlayer()->KilledMonsterCredit(target->GetEntry());
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_ctu_snap_effect_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ctu_snap_effect_SpellScript();
    }
};

//Orc Scout
class npc_orc_scout : public CreatureScript
{
    public:
        npc_orc_scout() : CreatureScript("npc_orc_scout") { }

    struct npc_orc_scoutAI : public Scripted_NoMovementAI
    {
        npc_orc_scoutAI(Creature* creature) : Scripted_NoMovementAI(creature) { }

        uint32 check;
        void Reset()
        {
            check = 5000;
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            // God mode.
            damage = 0;
        }

        void UpdateAI(uint32 const diff)
        {
            UpdateVictim();

            if (check <= diff)
            {
                Unit * target = me->getVictim();
                if (!target)
                {
                    target = me->SelectNearestTargetInAttackDistance(50.0f);
                    AttackStart(target);
                }
                if (target)
                    DoCast(target, SPELL_SHOOT);
                check = 5000;
            }else
                check -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_orc_scoutAI(creature);
    }
};

//Strangle Vine
class npc_strangle_vine : public CreatureScript
{
    public:
        npc_strangle_vine() : CreatureScript("npc_strangle_vine") { }

    struct npc_strangle_vineAI : public Scripted_NoMovementAI
    {
        npc_strangle_vineAI(Creature* creature) : Scripted_NoMovementAI(creature) { }

        void Reset()
        {

        }
        void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply)
        {
            if (passenger->GetTypeId() != TYPEID_UNIT)
                return;
            if (apply)
                passenger->CastSpell(passenger, SPELL_VISUAL_VILE_CAPTURE, true);
            else
            {
                passenger->RemoveAura(SPELL_VISUAL_VILE_CAPTURE);
                sCreatureTextMgr->SendChat(passenger->ToCreature(), TEXT_GENERIC_0);
                passenger->HandleEmoteCommand(EMOTE_SAFE_ORC);
                passenger->GetMotionMaster()->MovePoint(0, 606.6343f, 2785.689f, 88.11332f);
                passenger->ToCreature()->DespawnOrUnsummon(15000);
            }
                
        }

        void UpdateAI(uint32 const diff)
        {
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_strangle_vineAI(creature);
    }
};

//Weed Whacker 
class spell_weed_whacker : public SpellScriptLoader
{
public:
    spell_weed_whacker() : SpellScriptLoader("spell_weed_whacker") { }

    class spell_weed_whacker_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_weed_whacker_SpellScript);

        void HandleScript(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            Unit* caster =  GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            if (caster->GetTypeId() == TYPEID_PLAYER &&
                caster->ToPlayer()->GetQuestStatus(QUEST_WEED_WHACKER) != QUEST_STATUS_INCOMPLETE)
                return;

            if (caster->HasAura(SPELL_WEED_WHACKER))
            {
                caster->RemoveAura(SPELL_WEED_WHACKER);
                caster->RemoveAura(SPELL_WEED_WHACKER_BUF);
            }else
            {
                /*caster->CastSpell(caster, 68216, true); // summon bunny
                //bunny cast channel on plr 68214 
                //if (TempSummon* summon = caster->GetMap()->SummonCreature(35903, *caster))
                //{
                //    summon->CastSpell(caster, 68214, true);
                //    summon->CastSpell(caster, 68217, true);
                //}
                WorldPacket data(SMSG_FORCE_SET_VEHICLE_REC_ID, 16);
                data.WriteGuidMask<1, 5, 0, 6, 4, 3, 7, 2>(caster->GetObjectGuid());
                data.WriteGuidBytes<7, 2, 5, 6, 4>(caster->GetObjectGuid());
                data << uint32(493);
                data.WriteGuidBytes<3, 1, 0>(caster->GetObjectGuid());
                data << uint32(534);          //unk
                caster->SendMessageToSet(&data, true);

                data.Initialize(SMSG_PLAYER_VEHICLE_DATA, 8 + 1 + 4);
                data << uint32(493);
                data.WriteGuidMask<5, 3, 6, 2, 1, 4, 0, 7>(caster->GetObjectGuid());
                data.WriteGuidBytes<6, 0, 1, 3, 5, 7, 2, 4>(caster->GetObjectGuid());
                caster->SendMessageToSet(&data, true);*/

                caster->CastSpell(caster, SPELL_WEED_WHACKER, true);
                caster->CastSpell(caster, SPELL_WEED_WHACKER_BUF, true);
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_weed_whacker_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_weed_whacker_SpellScript();
    }
};

/*
ServerToClient: SMSG_MONSTER_MOVE (0x6E17) Length: 182 ConnectionIndex: 2 Time: 08/07/2012 10:46:08.603 Type: Unknown Opcode Type Number: 69708
GUID: Full: 0xF1508EE900066DF4 Type: Vehicle Entry: 36585 Low: 421364
Toggle AnimTierInTrans: False
Position: X: 867.7984 Y: 2821.093 Z: 108.3891
Move Ticks: 33381247
Spline Type: Normal (0)
Spline Flags: Walkmode, Parabolic (35651584)
Move Time: 47733
Vertical Speed: 7.431631
Async-time in ms: 45413
Waypoints: 32
Waypoint Endpoint: X: 1079.168 Y: 3239.45 Z: 81.53538
[0] Waypoint: X: 1080.983 Y: 3235.022 Z: 82.71225
[1] Waypoint: X: 1080.983 Y: 3222.522 Z: 86.21225
[2] Waypoint: X: 1071.483 Y: 3201.772 Z: 88.21225
[3] Waypoint: X: 1064.483 Y: 3190.772 Z: 88.96225
[4] Waypoint: X: 1047.733 Y: 3181.772 Z: 89.71225
[5] Waypoint: X: 1032.233 Y: 3166.522 Z: 89.71225
[6] Waypoint: X: 1020.983 Y: 3152.772 Z: 87.71225
[7] Waypoint: X: 1009.983 Y: 3138.272 Z: 83.96225
[8] Waypoint: X: 1000.733 Y: 3123.272 Z: 81.21225
[9] Waypoint: X: 991.7334 Y: 3115.522 Z: 80.21225
[10] Waypoint: X: 977.4834 Y: 3112.022 Z: 79.71225
[11] Waypoint: X: 954.7334 Y: 3111.522 Z: 80.71225
[12] Waypoint: X: 936.7334 Y: 3109.522 Z: 81.46225
[13] Waypoint: X: 921.7334 Y: 3100.772 Z: 79.46225
[14] Waypoint: X: 910.2334 Y: 3091.522 Z: 77.71225
[15] Waypoint: X: 891.2334 Y: 3076.522 Z: 75.71225
[16] Waypoint: X: 875.9834 Y: 3058.272 Z: 71.96225
[17] Waypoint: X: 867.2334 Y: 3040.272 Z: 68.21225
[18] Waypoint: X: 859.2334 Y: 3017.522 Z: 66.46225
[19] Waypoint: X: 861.9834 Y: 2996.522 Z: 65.96225
[20] Waypoint: X: 868.4834 Y: 2978.772 Z: 64.96225
[21] Waypoint: X: 879.2334 Y: 2964.772 Z: 63.96225
[22] Waypoint: X: 898.9834 Y: 2944.772 Z: 64.46225
[23] Waypoint: X: 916.2334 Y: 2930.272 Z: 64.96225
[24] Waypoint: X: 925.7334 Y: 2916.772 Z: 65.96225
[25] Waypoint: X: 926.2334 Y: 2893.772 Z: 69.46225
[26] Waypoint: X: 911.9834 Y: 2882.772 Z: 71.21225
[27] Waypoint: X: 897.9834 Y: 2867.772 Z: 74.46225
[28] Waypoint: X: 894.7334 Y: 2858.522 Z: 77.21225
[29] Waypoint: X: 892.2334 Y: 2848.022 Z: 83.96225
[30] Waypoint: X: 886.2334 Y: 2836.772 Z: 92.21225
*/
class npc_bastia : public CreatureScript
{
public:
    npc_bastia() : CreatureScript("npc_bastia") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_bastiaAI (creature);
    }

    struct npc_bastiaAI : public npc_escortAI
    {
        npc_bastiaAI(Creature* creature) : npc_escortAI(creature) {}

        bool PlayerOn;
        void Reset()
        {
             PlayerOn       = false;
        }

        void OnCharmed(bool /*apply*/)
        {
        }


        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (!apply || who->GetTypeId() != TYPEID_PLAYER)
                return;

             PlayerOn = true;
             Start(false, true, who->GetGUID());
        }

        void WaypointReached(uint32 i)
        {
            switch(i)
            {
                case 20:
                    if (Player* player = GetPlayerForEscort())
                    {
                        SetEscortPaused(true);
                        player->ExitVehicle();
                        player->SetClientControl(me, 1);
                    }
                    break;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            npc_escortAI::UpdateAI(diff);
            
            if (PlayerOn)
            {
                if (Player* player = GetPlayerForEscort())
                    player->SetClientControl(me, 0);
                PlayerOn = false;
            }
        }
    };
};

void AddSC_lost_isle()
{
    new npc_gizmo();
    new npc_doc_zapnnozzle();
    new npc_foreman_dampwick();
    new npc_frightened_miner();
    new spell_photo_capturing();
    new spell_ctu_snap_effect();
    new npc_orc_scout();
    new npc_strangle_vine();
    new spell_weed_whacker();
    new npc_bastia();
}