/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef __RedisSaveBuilder_H
#define __RedisSaveBuilder_H

class PlayerSave
{
    public:
        PlayerSave(Player* player) : m_player(player) {}
        ~PlayerSave() {}

        void SaveToDB();
        void SaveItem();

    private:

        Player* m_player;
};

class RedisBuilderMgr
{
    private:
        RedisBuilderMgr();
        ~RedisBuilderMgr();

    public:
        static RedisBuilderMgr* instance()
        {
            static RedisBuilderMgr instance;
            return &instance;
        }
        std::string BuildString(Json::Value& data);
        bool LoadFromRedis(const RedisValue* v, Json::Value& data);
        bool LoadFromRedisArray(const RedisValue* v, std::vector<RedisValue>& data);
        bool LoadFromString(std::string string_data, Json::Value& data);

        void InitRedisKey();

        bool CheckKey(const char* _key);

        const char* GetGuidKey() { return queryGuidKey.c_str(); }
        const char* GetPetKey() { return petKey.c_str(); }
        const char* GetBracketKey() { return bracketKey.c_str(); }
        const char* GetAucItemKey() { return aucItemKey.c_str(); }
        const char* GetAucKey() { return aucKey.c_str(); }
        const char* GetGuildKey() { return guildKey.c_str(); }
        const char* GetGuildFKey() { return guildFKey.c_str(); }
        const char* GetGuildFMKey() { return guildFMKey.c_str(); }
        const char* GetGroupKey() { return groupKey.c_str(); }
        const char* GetGroupMemberKey() { return groupMemberKey.c_str(); }
        const char* GetGroupInstanceKey() { return groupInstanceKey.c_str(); }
        const char* GetChallengeKey() { return challengeKey.c_str(); }
        const char* GetTicketKey() { return ticketKey.c_str(); }
        const char* GetMailsKey() { return mailsKey.c_str(); }

        Json::Reader Reader;
        Json::FastWriter Builder;
        //Value ItemData;

    private:

        std::string queryGuidKey;
        std::string petKey;
        std::string bracketKey;
        std::string aucItemKey;
        std::string aucKey;
        std::string guildKey;
        std::string guildFKey;
        std::string guildFMKey;
        std::string groupKey;
        std::string groupMemberKey;
        std::string groupInstanceKey;
        std::string challengeKey;
        std::string ticketKey;
        std::string mailsKey;
};

#define sRedisBuilderMgr RedisBuilderMgr::instance()
#endif
