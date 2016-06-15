/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptPCH.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"

class CustomRewardScript : public PlayerScript
{
public:
    CustomRewardScript() : PlayerScript("CustomRewardScript") { }

    void OnLogin(Player* player)
    {
        if(!player)
            return;
        uint32 owner_guid = player->GetGUIDLow();
        ChatHandler chH = ChatHandler(player);

        //type:
        // 1 achievement
        // 2 title
        // 3 item
        // 4 quest
        // 5 give 1 level
        // 6 level up to
        // 7 spell
        // 8 Remove pvp items from sliver char
        // 9 Remove items from char from loot
        // 11 Remove title
        // 12 Add char to char enum
        bool rewarded = false;
        QueryResult result = CharacterDatabase.PQuery("SELECT guid, type, id, count FROM `character_reward` WHERE owner_guid = '%u'", owner_guid);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 guid = fields[0].GetUInt32();
                uint32 type = fields[1].GetUInt32();
                uint32 id = fields[2].GetUInt32();
                uint32 count = fields[3].GetUInt32();
                rewarded = false;
                switch (type)
                {
                    case 1: // Add achievement to char
                    {
                        if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(id))
                            player->CompletedAchievement(achievementEntry);
                        rewarded = true;
                    }
                    break;
                    case 2: // Add title to char
                    {
                        if(CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id))
                        {
                            char const* targetName = player->GetName();
                            char titleNameStr[80];
                            snprintf(titleNameStr, 80, titleInfo->name, targetName);
                            player->SetTitle(titleInfo);
                            chH.PSendSysMessage(LANG_TITLE_ADD_RES, id, titleNameStr, targetName);
                            rewarded = true;
                        }
                    }
                    break;
                    case 3: // Add items to char
                    {
                        if(ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(id))
                        {
                            //Adding items
                            uint32 noSpaceForCount = 0;

                            // check space and find places
                            ItemPosCountVec dest;
                            InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count, &noSpaceForCount);
                            if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
                                count -= noSpaceForCount;

                            if (count == 0 || dest.empty())                         // can't add any
                            {
                                chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                                break;
                            }

                            Item* item = player->StoreNewItem(dest, id, true, Item::GenerateItemRandomPropertyId(id));

                            if (count > 0 && item)
                                player->SendNewItem(item, count, false, true);

                            if (noSpaceForCount > 0)
                            {
                                chH.PSendSysMessage(LANG_ITEM_CANNOT_CREATE, id, noSpaceForCount);
                                CharacterDatabase.PExecute("UPDATE character_reward SET count = count - %u WHERE guid = %u", count, guid);
                            }
                            else
                                rewarded = true;
                        }
                    }
                    break;
                    case 4: // Add quest to char
                    {
                        //not implemented
                    }
                    break;
                    case 5: // Add 1 level to char
                    {
                        //not implemented
                    }
                    break;
                    case 6: // Level up to
                    {
                        //not implemented
                    }
                    break;
                    case 7: // Learn spell
                    {
                        //not implemented
                    }
                    break;
                    case 8:
                    {
                        CleanItems(player);
                        //player->SaveToDB();
                        rewarded = true;
                    }
                    break;
                    case 9:
                    {
                        FindItem(player, id, count);
                        //player->SaveToDB();
                        rewarded = true;
                    }
                    break;
                    case 11: // Remove title from char
                    {
                        if(CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(id))
                        {
                            char const* targetName = player->GetName();
                            char titleNameStr[80];
                            snprintf(titleNameStr, 80, titleInfo->name, targetName);
                            player->SetTitle(titleInfo, true);
                            chH.PSendSysMessage(LANG_TITLE_REMOVE_RES, id, titleNameStr, targetName);
                            rewarded = true;
                        }
                    }
                    break;
                    case 12:
                    {
                        sWorld->AddCharacterNameData(guid, player->GetName(), player->getGender(), player->getRace(), player->getClass(), player->getLevel());
                        rewarded = true;
                    }
                    break;
                    default:
                        break;
                }
                if(rewarded)
                    CharacterDatabase.PExecute("DELETE FROM character_reward WHERE guid = %u", guid);
            }while (result->NextRow());
        }
        else
        {
            if (AchievementEntry const *achiev = sAchievementStore.LookupEntry(252))
                if (player->GetAchievementMgr().IsCompletedAchievement(achiev, player))
                    player->CompletedAchievement(achiev);

            // Quest "A Test of Valor"
            if (player->GetAchievementMgr().HasAchieved(8030) || player->GetAchievementMgr().HasAchieved(8031))
                player->KilledMonsterCredit(69145, 0);

            if(QueryResult share_result = CharacterDatabase.PQuery("SELECT * FROM `character_share` WHERE guid = '%u'", owner_guid))
            {
                uint32 totaltime = player->GetTotalPlayedTime();
                bool update = false;
                Field* fields = share_result->Fetch();
                uint32 bonus1 = fields[1].GetBool();
                uint32 bonus2 = fields[2].GetBool();
                uint32 bonus3 = fields[3].GetBool();
                uint32 bonus4 = fields[4].GetBool();
                uint32 bonus5 = fields[5].GetBool();
                uint32 bonus6 = fields[6].GetBool();
                uint32 bonus7 = fields[7].GetBool();
                uint32 bonus8 = fields[8].GetBool();
                uint32 bonus9 = fields[9].GetBool();
                uint32 bonus10 = fields[10].GetBool();
                uint32 bonus11 = fields[11].GetBool();

                if (sWorld->getBoolConfig(CONFIG_SHARE_ENABLE))
                {
                    if(!bonus1 && totaltime >= (5 * HOUR))
                    {
                        player->ModifyMoney(2000000);
                        update = true;
                        bonus1 = true;
                    }
                    if(!bonus2 && totaltime >= (10 * HOUR))
                    {
                        player->ModifyMoney(4000000);
                        update = true;
                        bonus2 = true;
                    }
                    if(!bonus3 && totaltime >= (20 * HOUR))
                    {
                        player->ModifyMoney(8000000);
                        update = true;
                        bonus3 = true;
                    }
                    if(!bonus4 && totaltime >= (50 * HOUR))
                    {
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 72068);
                        update = true;
                        bonus4 = true;
                    }
                }

                if(!bonus5 && totaltime >= (100 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 49663);
                    update = true;
                    bonus5 = true;
                }
                if(!bonus6 && totaltime >= (500 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 83086);
                    update = true;
                    bonus6 = true;
                }
                if(!bonus7 && totaltime >= (1000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 106246);
                    update = true;
                    bonus7 = true;
                }
                if(!bonus8 && totaltime >= (2500 * HOUR))
                {
                    if(player->GetTeam() == HORDE)
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 76902);
                    else
                        CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 76889);
                    update = true;
                    bonus8 = true;
                }
                if(!bonus9 && totaltime >= (5000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 69846);
                    update = true;
                    bonus9 = true;
                }
                if(!bonus10 && totaltime >= (9000 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 21176);
                    update = true;
                    bonus10 = true;
                }
                if(!bonus11 && totaltime >= (250 * HOUR))
                {
                    CharacterDatabase.PQuery("INSERT INTO `character_reward` (`owner_guid`, `type`, `id`, `count`) VALUES('%u','3','%u','1')", owner_guid, 68385);
                    update = true;
                    bonus11 = true;
                }
                if(update)
                    CharacterDatabase.PQuery("UPDATE `character_share` SET `bonus1` = '%u', `bonus2` = '%u', `bonus3` = '%u', `bonus4` = '%u', `bonus5` = '%u', `bonus6` = '%u', `bonus7` = '%u', `bonus8` = '%u', `bonus9` = '%u', `bonus10` = '%u', `bonus11` = '%u' WHERE guid = '%u'", bonus1, bonus2, bonus3, bonus4, bonus5, bonus6, bonus7, bonus8, bonus9, bonus10, bonus11, owner_guid);
            }
            else
                CharacterDatabase.PQuery("INSERT INTO `character_share` (`guid`, `bonus1`, `bonus2`, `bonus3`, `bonus4`, `bonus5`, `bonus6`, `bonus7`, `bonus8`, `bonus9`, `bonus10`, `bonus11`) values('%u','0','0','0','0','0','0','0','0','0','0','0')", owner_guid);
        }
    }

    void FindItem(Player* player, uint32 entry, uint32 count)
    {
        // in inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if(pItem->GetEntry() == entry)
                {
                    count = ItemDel(pItem, player, count);
                    if(!count)
                        return;
                }

        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if(pItem->GetEntry() == entry)
                        {
                            count = ItemDel(pItem, player, count);
                            if(!count)
                                return;
                        }

        for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if(pItem->GetEntry() == entry)
                  {
                      count = ItemDel(pItem, player, count);
                      if(!count)
                          return;
                  }
            }

        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if(pItem->GetEntry() == entry)
                {
                    count = ItemDel(pItem, player, count);
                    if(!count)
                        return;
                }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if(pItem->GetEntry() == entry)
                        {
                            count = ItemDel(pItem, player, count);
                            if(!count)
                                return;
                        }
    }

    uint32 ItemDel(Item* _item, Player* player, uint32 count)
    {
        uint32 tempcount = count;
        QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE itemguid = '%u'", _item->GetGUIDLow());
        if(!result)
        {
            ChatHandler chH = ChatHandler(player);
            if (_item->GetCount() >= count)
                count = 0;
            else
                count -=_item->GetCount();

            player->DestroyItemCount(_item, tempcount, true);
            //sLog->outError("ItemDel item delete %u, count %u, tempcount %u", _item->GetEntry(), count, tempcount);
            chH.PSendSysMessage(20021, sObjectMgr->GetItemTemplate(_item->GetEntry())->Name1.c_str());
        }
        return count;
    }

    void CleanItems(Player* player)
    {
        // in inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        ItemIfExistDel(pItem, player);

        for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ItemIfExistDel(pItem, player);

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag* pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        ItemIfExistDel(pItem, player);
    }

    void ItemIfExistDel(Item* _item, Player* player)
    {
        switch (_item->GetEntry())
        {
            case 102594:
            case 103365:
            case 102602:
            case 103501:
            case 102603:
            case 103503:
            case 103502:
            case 102608:
            case 103508:
            case 103339:
            case 102604:
            case 102600:
            case 103509:
            case 103404:
            case 103340:
            case 102609:
            case 102592:
            case 102590:
            case 102598:
            case 103510:
            case 102605:
            case 102612:
            case 103504:
            case 103341:
            case 102606:
            case 103461:
            case 102601:
            case 102607:
            case 103366:
            case 103406:
            case 102610:
            case 103336:
            case 102599:
            case 103338:
            case 102591:
            case 103350:
            case 102597:
            case 103403:
            case 102593:
            case 102596:
            case 103374:
            case 103507:
            case 103521:
            case 103468:
            case 103463:
            case 103422:
            case 102751:
            case 102726:
            case 102703:
            case 102667:
            case 103523:
            case 103470:
            case 103465:
            case 103424:
            case 102715:
            case 102681:
            case 102661:
            case 102622:
            case 103359:
            case 103358:
            case 103357:
            case 102733:
            case 102687:
            case 102666:
            case 103520:
            case 103467:
            case 103462:
            case 103421:
            case 102735:
            case 102725:
            case 102707:
            case 102615:
            case 102621:
            case 103522:
            case 103469:
            case 103464:
            case 103423:
            case 102648:
            case 102704:
            case 102755:
            case 102671:
            case 102673:
            case 102682:
            case 102750:
            case 103425:
            case 103466:
            case 103471:
            case 103524:
            case 102662:
            case 102684:
            case 102709:
            case 103351:
            case 103352:
            case 103353:
            case 102620:
            case 102686:
            case 102752:
            case 103354:
            case 103355:
            case 103356:
            case 102628:
            case 102634:
            case 102653:
            case 102710:
            case 102712:
            case 102776:
            case 103382:
            case 103390:
            case 103399:
            case 103430:
            case 103435:
            case 103477:
            case 102614:
            case 102720:
            case 102721:
            case 102727:
            case 102740:
            case 102763:
            case 103384:
            case 103392:
            case 103401:
            case 103433:
            case 103438:
            case 103475:
            case 102691:
            case 102711:
            case 102723:
            case 102754:
            case 103388:
            case 103397:
            case 103428:
            case 103474:
            case 102627:
            case 102657:
            case 102663:
            case 102675:
            case 102696:
            case 102739:
            case 103381:
            case 103389:
            case 103398:
            case 103429:
            case 103434:
            case 103476:
            case 102654:
            case 102656:
            case 102730:
            case 102761:
            case 102762:
            case 102767:
            case 103383:
            case 103391:
            case 103400:
            case 103431:
            case 103436:
            case 103478:
            case 102626:
            case 102658:
            case 102700:
            case 102731:
            case 102741:
            case 102777:
            case 103385:
            case 103393:
            case 103402:
            case 103432:
            case 103437:
            case 103479:
            case 102647:
            case 102694:
            case 102708:
            case 102775:
            case 103386:
            case 103395:
            case 103426:
            case 103472:
            case 102631:
            case 102660:
            case 102716:
            case 102760:
            case 103387:
            case 103396:
            case 103427:
            case 103473:
            case 102690:
            case 102693:
            case 102714:
            case 102718:
            case 103418:
            case 103487:
            case 103492:
            case 103498:
            case 102689:
            case 102717:
            case 102743:
            case 102759:
            case 103416:
            case 103485:
            case 103490:
            case 103496:
            case 102624:
            case 102665:
            case 102688:
            case 102753:
            case 103414:
            case 103415:
            case 103483:
            case 103484:
            case 102692:
            case 102737:
            case 102742:
            case 102774:
            case 103417:
            case 103486:
            case 103491:
            case 103497:
            case 102670:
            case 102719:
            case 102778:
            case 102781:
            case 103419:
            case 103488:
            case 103493:
            case 103499:
            case 102629:
            case 102637:
            case 102655:
            case 102734:
            case 103420:
            case 103489:
            case 103494:
            case 103500:
            case 102617:
            case 102645:
            case 102668:
            case 102677:
            case 103410:
            case 103411:
            case 103480:
            case 103495:
            case 102623:
            case 102646:
            case 102664:
            case 102729:
            case 103412:
            case 103413:
            case 103481:
            case 103482:
            case 102619:
            case 102635:
            case 102713:
            case 102779:
            case 103378:
            case 103441:
            case 103452:
            case 103527:
            case 102632:
            case 102676:
            case 102728:
            case 102747:
            case 103376:
            case 103439:
            case 103450:
            case 103525:
            case 102695:
            case 102702:
            case 102765:
            case 102771:
            case 103448:
            case 103449:
            case 103459:
            case 103460:
            case 102618:
            case 102630:
            case 102650:
            case 102722:
            case 103377:
            case 103440:
            case 103451:
            case 103526:
            case 102651:
            case 102732:
            case 102768:
            case 102780:
            case 103379:
            case 103442:
            case 103453:
            case 103528:
            case 102652:
            case 102685:
            case 102697:
            case 102744:
            case 103380:
            case 103443:
            case 103454:
            case 103529:
            case 102640:
            case 102724:
            case 102748:
            case 102764:
            case 103444:
            case 103445:
            case 103455:
            case 103456:
            case 102638:
            case 102639:
            case 102698:
            case 102745:
            case 103446:
            case 103447:
            case 103457:
            case 103458:
            case 102641:
            case 102678:
            case 102679:
            case 102736:
            case 102749:
            case 102757:
            case 102773:
            case 103345:
            case 103346:
            case 103367:
            case 103368:
            case 103369:
            case 103514:
            case 103515:
            case 102644:
            case 102649:
            case 102674:
            case 102683:
            case 102701:
            case 102746:
            case 102758:
            case 103348:
            case 103349:
            case 103370:
            case 103371:
            case 103372:
            case 103517:
            case 103518:
            case 102616:
            case 102625:
            case 102633:
            case 102636:
            case 102643:
            case 102659:
            case 102672:
            case 102680:
            case 102699:
            case 102706:
            case 102738:
            case 102766:
            case 103342:
            case 103347:
            case 103407:
            case 103408:
            case 103409:
            case 103505:
            case 103506:
            case 103511:
            case 103516:
            case 103530:
            case 103531:
            case 103532:
            case 102642:
            case 102669:
            case 102705:
            case 102756:
            case 102769:
            case 102770:
            case 102772:
            case 103343:
            case 103344:
            case 103360:
            case 103361:
            case 103362:
            case 103512:
            case 103513:
            case 102783:
            case 102786:
            case 103363:
            case 103373:
            case 102782:
            case 102784:
            case 102785:
            case 103364:
            case 103405:
            case 103519:            {
                QueryResult result = CharacterDatabase.PQuery("SELECT itemEntry FROM character_donate WHERE itemguid = '%u'", _item->GetGUIDLow());
                if(!result)
                {
                    ChatHandler chH = ChatHandler(player);
                    player->DestroyItem(_item->GetBagSlot(), _item->GetSlot(), true);
                    //sLog->outError("ItemIfExistDel item delete %u", _item->GetEntry());
                    chH.PSendSysMessage(20020, sObjectMgr->GetItemTemplate(_item->GetEntry())->Name1.c_str());
                }
            }
        }
    }
};

void AddSC_custom_reward()
{
    new CustomRewardScript();
}
