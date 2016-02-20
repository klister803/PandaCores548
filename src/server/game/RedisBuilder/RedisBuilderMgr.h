/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#ifndef __RedisSaveBuilder_H
#define __RedisSaveBuilder_H

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

        void InitRedisKey();

        bool CheckKey(char* _key);

        char* GetGuidKey() { return queryGuidKey; }
        char* GetPetKey() { return petKey; }
        char* GetBracketKey() { return bracketKey; }
        char* GetAucItemKey() { return aucItemKey; }
        char* GetAucKey() { return aucKey; }
        char* GetGuildKey() { return guildKey; }
        char* GetGuildFKey() { return guildFKey; }
        char* GetGuildFMKey() { return guildFMKey; }
        char* GetGroupKey() { return groupKey; }
        char* GetGroupMemberKey() { return groupMemberKey; }
        char* GetGroupInstanceKey() { return groupInstanceKey; }
        char* GetChallengeKey() { return challengeKey; }
        char* GetTicketKey() { return ticketKey; }

        Json::Reader Reader;
        Json::FastWriter Builder;
        //Value ItemData;

    private:

        char* queryGuidKey;
        char* petKey;
        char* bracketKey;
        char* aucItemKey;
        char* aucKey;
        char* guildKey;
        char* guildFKey;
        char* guildFMKey;
        char* groupKey;
        char* groupMemberKey;
        char* groupInstanceKey;
        char* challengeKey;
        char* ticketKey;
};

#define sRedisBuilderMgr RedisBuilderMgr::instance()


#endif
