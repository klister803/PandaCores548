/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
#define REDISASYNCCLIENT_REDISASYNCCLIENT_CPP

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "../redisclient.h"

RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
    : pimpl(boost::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
    pimpl->errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                      pimpl, _1);
}

RedisAsyncClient::~RedisAsyncClient()
{
    pimpl->close();
}

void RedisAsyncClient::connect(const boost::asio::ip::address &address,
                               unsigned short port,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    connect(endpoint, handler);
}

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    pimpl->socket.async_connect(endpoint, boost::bind(&RedisClientImpl::handleAsyncConnect,
                                                      pimpl, _1, handler));
}

void RedisAsyncClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    pimpl->errorHandler = handler;
}

void RedisAsyncClient::command(uint64 guid, const std::string &s, const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(1);
        items[0] = s;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(2);
        items[0] = cmd;
        items[1] = arg1;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    //fprintf(stderr, "RedisAsyncClient::command: guid: %i id %i\n", guid, boost::this_thread::get_id());

    if(stateValid())
    {
        std::vector<RedisBuffer> items(3);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items),
                    guid,
                    handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(4);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, 
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(5);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(6);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5,
                          const RedisBuffer &arg6,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(7);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const RedisBuffer &arg1,
                          const RedisBuffer &arg2, const RedisBuffer &arg3,
                          const RedisBuffer &arg4, const RedisBuffer &arg5,
                          const RedisBuffer &arg6, const RedisBuffer &arg7,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(8);
        items[0] = cmd;
        items[1] = arg1;
        items[2] = arg2;
        items[3] = arg3;
        items[4] = arg4;
        items[5] = arg5;
        items[6] = arg6;
        items[7] = arg7;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::command(uint64 guid, const std::string &cmd, const std::list<RedisBuffer> &args,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    if(stateValid())
    {
        std::vector<RedisBuffer> items(1);
        items[0] = cmd;

        items.reserve(1 + args.size());

        std::copy(args.begin(), args.end(), std::back_inserter(items));
        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), guid, handler));
    }
}

void RedisAsyncClient::singleShotSubscribe(const std::string &channel,
                                      const boost::function<void(const std::vector<char> &msg)> &msgHandler,
                                      const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static const std::string subscribeStr = "SUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        std::vector<RedisBuffer> items(2);
        items[0] = subscribeStr;
        items[1] = channel;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), 0, handler));
        pimpl->singleShotMsgHandlers.insert(std::make_pair(channel, msgHandler));
        pimpl->state = RedisClientImpl::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}

void RedisAsyncClient::publish(const std::string &channel, const RedisBuffer &msg,
                          const boost::function<void(const RedisValue &v, uint64 guid)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected );

    static const std::string publishStr = "PUBLISH";

    if( pimpl->state == RedisClientImpl::Connected )
    {
        std::vector<RedisBuffer> items(3);

        items[0] = publishStr;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl,
                    pimpl->makeCommand(items), 0, handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}

bool RedisAsyncClient::stateValid() const
{
    assert( pimpl->state == RedisClientImpl::Connected );

    if( pimpl->state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}

#endif // REDISASYNCCLIENT_REDISASYNCCLIENT_CPP
