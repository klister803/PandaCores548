#pragma once
#ifndef _INSTANCE_DRAGON_SOUL_H
#define _INSTANCE_DRAGON_SOUL_H

#include "dragon_soul.h"

struct instance_dragon_soul_InstanceMapScript : public InstanceScript
{
    instance_dragon_soul_InstanceMapScript(Map* map);
    ~instance_dragon_soul_InstanceMapScript();

    // Called when the instance is created, not loaded.
    void Initialize() ;

    // Called when the instance is loaded.
    void Load(const char* data) ;
    std::string GetSaveData() ;

    void OnCreatureCreate(Creature* npc) ;
    void OnGameObjectCreate(GameObject* go) ;
    void OnUnitDeath(Unit* unit) ;

    bool IsEncounterInProgress() const ;

    uint32 GetData(uint32 uiType) ;
    void SetData(uint32 uiType, uint32 uiData) ;

    Creature* GetCreature(uint32 entry);

    void WhisperToAllPlayerInZone(int32 TextId, Creature* sender);

private:
    ObjectGuid morchok;

    ObjectGuid valeera;
    ObjectGuid eiendormi;

    uint32 m_auiEncounter[MAX_ENCOUNTER];
};

#endif
