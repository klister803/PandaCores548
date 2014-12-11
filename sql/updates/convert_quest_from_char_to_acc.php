<?php
$link = mysql_connect("localhost","root","root");
mysql_select_db("trinchar",$link) or die("Could not select database");
mysql_query ("set character_set_client='utf8'");
mysql_query ("set character_set_results='utf8'");
mysql_query ("set collation_connection='utf8_general_ci'");

$result_foreach = mysql_query("SELECT guid,account FROM `characters`;",$link) or die("Query failed : " . mysql_error());
while ($request_acc = mysql_fetch_array($result_foreach, MYSQL_ASSOC))
{
    $guid = $request_acc["guid"];
    $account = $request_acc["account"];
    if(!$account)
        continue;
    mysql_query("UPDATE character_queststatus SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_daily SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_rewarded SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_seasonal SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_weekly SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
}

$result_foreach = mysql_query("SELECT guid, deleteInfos_Account FROM `characters` WHERE account = 0;",$link) or die("Query failed : " . mysql_error());
while ($request_acc = mysql_fetch_array($result_foreach, MYSQL_ASSOC))
{
    $guid = $request_acc["guid"];
    $account = $request_acc["deleteInfos_Account"];
    if($account == NULL || !$account)
        continue;
    mysql_query("UPDATE character_queststatus SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_daily SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_rewarded SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_seasonal SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
    mysql_query("UPDATE character_queststatus_weekly SET `account` = '$account' WHERE `guid` = $guid;",$link) or die("Query failed : " . mysql_error());
}
?>