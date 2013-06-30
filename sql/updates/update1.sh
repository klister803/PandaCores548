USER=root
PASS=rWwubSTy3JE1Oem
DBNAME=trinworld505
DBHOST=localhost

mysqldump -h $DBHOST -u $USER --password=$PASS $DBNAME > l2jdb_full_backup.sql

for tab in world/2012/*.sql
do
                echo Loading $tab ...
                mysql -f -h $DBHOST -u $USER --password=$PASS -D $DBNAME < $tab >> update.log
done