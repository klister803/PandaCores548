delete from creature_template where entry = 90903;
update creature_template set modelid2 = 0, modelid3 = 0, minlevel = 90, maxlevel = 90, unit_flags = 34080768, inhabittype = 7, flags_extra = 128, scriptName = 'npc_amberbeam_stalker' where entry = 62510;
update creature_template set flags_extra = 536870912 where entry = 62711;

update spell_linked_spell set spell_trigger = -122784 where spell_trigger = 122784;