#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "Vehicle.h"

class AreaTrigger_at_bassin_curse : public AreaTriggerScript
{
    public:
        AreaTrigger_at_bassin_curse() : AreaTriggerScript("AreaTrigger_at_bassin_curse")
        {}

        enum eTriggers
        {
            AREA_CRANE              = 6991,
            AREA_SKUNK              = 6988,
            AREA_FROG               = 6987,
            AREA_FROG_EXIT          = 6986,
            AREA_TURTLE             = 7012,
            AREA_CROCODILE          = 6990
        };

        enum eSpells
        {
            SPELL_FROG              = 102938,
            SPELL_SKUNK             = 102939,
            SPELL_TURTLE            = 102940,
            SPELL_CRANE             = 102941,
            SPELL_CROCODILE         = 102942,
        };

        void AddOrRemoveSpell(Player* player, uint32 spellId)
        {
            RemoveAllSpellsExcept(player, spellId);

            if (!player->HasAura(spellId))
            {
                if (!player->IsOnVehicle())
                    player->AddAura(spellId, player);
            }
            else
                player->RemoveAurasDueToSpell(spellId);
        }

        void RemoveAllSpellsExcept(Player* player, uint32 spellId)
        {
            uint32 spellTable[5] = {SPELL_FROG, SPELL_SKUNK, SPELL_TURTLE, SPELL_CRANE, SPELL_CROCODILE};

            for (uint8 i = 0; i < 5; ++i)
                if (spellId != spellTable[i])
                    player->RemoveAurasDueToSpell(spellTable[i]);
        }

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            switch(trigger->id)
            {
                case AREA_CRANE:     AddOrRemoveSpell(player, SPELL_CRANE);     break;
                case AREA_SKUNK:     AddOrRemoveSpell(player, SPELL_SKUNK);     break;
                case AREA_FROG:      AddOrRemoveSpell(player, SPELL_FROG);      break;
                case AREA_FROG_EXIT: RemoveAllSpellsExcept(player, 0);          break;
                case AREA_TURTLE:    AddOrRemoveSpell(player, SPELL_TURTLE);    break;
                case AREA_CROCODILE: AddOrRemoveSpell(player, SPELL_CROCODILE); break;
            }

            return true;
        }
};

class vehicle_balance_pole : public VehicleScript
{
    public:
        vehicle_balance_pole() : VehicleScript("vehicle_balance_pole") {}

        void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 /*seatId*/)
        {
            if (passenger->HasAura(102938))
                passenger->ExitVehicle();
        }

        /*void OnRemovePassenger(Vehicle* veh, Unit* passenger)
        {
            if (veh->GetBase()->GetPositionZ() == 116.521004f) // Hack
                if (passenger->IsOnVehicle()) // Maybe the player
                    passenger->AddAura(102938, passenger);
        }*/
};

class mob_tushui_monk : public CreatureScript
{
public:
    mob_tushui_monk() : CreatureScript("mob_tushui_monk") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_tushui_monkAI(creature);
    }

    struct mob_tushui_monkAI : public ScriptedAI
    {
        mob_tushui_monkAI(Creature* creature) : ScriptedAI(creature)
        {}

        void Reset()
        {
            std::list<Creature*> poleList;
            GetCreatureListWithEntryInGrid(poleList, me, 54993, 25.0f);

            if (poleList.empty())
            {
                me->DespawnOrUnsummon(1000);
                return;
            }

            JadeCore::Containers::RandomResizeList(poleList, 1);

            for (auto creature: poleList)
                me->EnterVehicle(creature);

            me->setFaction(2357);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->ExitVehicle();
            me->DespawnOrUnsummon(1000);
        }
    };
};

class spell_rock_jump: public SpellScriptLoader
{
    public:
        spell_rock_jump() : SpellScriptLoader("spell_rock_jump") { }

        class spell_rock_jump_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rock_jump_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetPositionZ() < 90.0f)
                        caster->GetMotionMaster()->MoveJump(1045.36f, 2848.47f, 91.38f, 10.0f, 10.0f);
                    else if (caster->GetPositionZ() < 92.0f)
                        caster->GetMotionMaster()->MoveJump(1054.42f, 2842.65f, 92.96f, 10.0f, 10.0f);
                    else if (caster->GetPositionZ() < 94.0f)
                        caster->GetMotionMaster()->MoveJump(1063.66f, 2843.49f, 95.50f, 10.0f, 10.0f);
                    else
                    {
                        caster->GetMotionMaster()->MoveJump(1078.42f, 2845.07f, 95.16f, 10.0f, 10.0f);

                        if (caster->ToPlayer())
                            caster->ToPlayer()->KilledMonsterCredit(57476);
                    }
                }
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_rock_jump_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rock_jump_SpellScript();
        }
};

Position rocksPos[4] =
{
    {1102.05f, 2882.11f, 94.32f, 0.11f},
    {1120.01f, 2883.20f, 96.44f, 4.17f},
    {1128.09f, 2859.44f, 97.64f, 2.51f},
    {1111.52f, 2849.84f, 94.84f, 1.94f}
};

class mob_shu_water_spirit : public CreatureScript
{
public:
    mob_shu_water_spirit() : CreatureScript("mob_shu_water_spirit") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shu_water_spiritAI(creature);
    }

    struct mob_shu_water_spiritAI : public ScriptedAI
    {
        mob_shu_water_spiritAI(Creature* creature) : ScriptedAI(creature)
        {}

        EventMap _events;
        uint8 actualPlace;

        uint64 waterSpoutGUID;

        enum eShuSpells
        {
            SPELL_WATER_SPOUT_SUMMON    = 116810,
            SPELL_WATER_SPOUT_WARNING   = 116695,
            SPELL_WATER_SPOUT_EJECT     = 116696,
            SPELL_WATER_SPOUT_VISUAL    = 117057,
        };

        enum eEvents
        {
            EVENT_CHANGE_PLACE          = 1,
            EVENT_SUMMON_WATER_SPOUT    = 2,
            EVENT_WATER_SPOUT_EJECT     = 3,
            EVENT_WATER_SPOUT_DESPAWN   = 4,
        };

        void Reset()
        {
            _events.Reset();
            actualPlace = 0;
            waterSpoutGUID = 0;

            _events.ScheduleEvent(EVENT_CHANGE_PLACE, 5000);
        }

        void MovementInform(uint32 typeId, uint32 pointId)
        {
            if (typeId != EFFECT_MOTION_TYPE)
                return;

            if (pointId == 1)
            {
                me->RemoveAurasDueToSpell(SPELL_WATER_SPOUT_WARNING);
                if (Player* player = me->SelectNearestPlayerNotGM(20.0f))
                {
                    me->SetOrientation(me->GetAngle(player));
                    me->SetFacingToObject(player);
                    _events.ScheduleEvent(EVENT_SUMMON_WATER_SPOUT, 2000);
                }
                else
                    _events.ScheduleEvent(EVENT_CHANGE_PLACE, 5000);
            }
        }

        void JustSummoned(Creature* summon)
        {
            if (summon->GetEntry() == 60488)
            {
                waterSpoutGUID = summon->GetGUID();
                summon->AddAura(SPELL_WATER_SPOUT_WARNING, summon);
            }
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            if (summon->GetEntry() == 60488)
                waterSpoutGUID = 0;
        }

        Creature* getWaterSpout(uint64 guid)
        {
            return me->GetMap()->GetCreature(guid);
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_CHANGE_PLACE:
                {
                    uint8 newPlace = 0;

                    do { newPlace = urand(0, 3); } while (newPlace == actualPlace);

                    me->GetMotionMaster()->MoveJump(rocksPos[newPlace].GetPositionX(), rocksPos[newPlace].GetPositionY(), rocksPos[newPlace].GetPositionZ(), 10.0f, 10.0f, 1);
                    me->AddAura(SPELL_WATER_SPOUT_WARNING, me); // Just visual
                    actualPlace = newPlace;
                    break;
                }
                case EVENT_SUMMON_WATER_SPOUT:
                {
                    float x = 0.0f, y = 0.0f;
                    GetPositionWithDistInOrientation(me, 5.0f, me->GetOrientation() + frand(-M_PI, M_PI), x, y);
                    me->CastSpell(x, y, 92.189629f, SPELL_WATER_SPOUT_SUMMON, false);
                    _events.ScheduleEvent(EVENT_WATER_SPOUT_EJECT, 7500);
                    break;
                }
                case EVENT_WATER_SPOUT_EJECT:
                {
                    if (Creature* waterSpout = getWaterSpout(waterSpoutGUID))
                    {
                        std::list<Player*> playerList;
                        GetPlayerListInGrid(playerList, waterSpout, 1.0f);

                        for (auto player: playerList)
                            player->CastSpell(player, SPELL_WATER_SPOUT_EJECT, true);

                        waterSpout->CastSpell(waterSpout, SPELL_WATER_SPOUT_VISUAL, true);
                    }
                    _events.ScheduleEvent(EVENT_WATER_SPOUT_DESPAWN, 3000);
                    break;
                }
                case EVENT_WATER_SPOUT_DESPAWN:
                {
                    if (Creature* waterSpout = getWaterSpout(waterSpoutGUID))
                        waterSpout->DespawnOrUnsummon();

                    _events.ScheduleEvent(EVENT_CHANGE_PLACE, 2000);
                    break;
                }
            }
        }
    };
};

void AddSC_WanderingIsland_East()
{
    new AreaTrigger_at_bassin_curse();
    new vehicle_balance_pole();
    new mob_tushui_monk();
    new spell_rock_jump();
    new mob_shu_water_spirit();
}
