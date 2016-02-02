/*
* Copyright (C) 2008-2016 UwowCore <http://uwow.biz/>
*/

#include "RedisEnv.h"
#include "RedisWorker.h"
#include "RedisOperation.h"
#include "RedisConnection.h"
#include "ProducerConsumerQueue.h"

RedisWorker::RedisWorker(ProducerConsumerQueue<RedisOperation*>* newQueue, RedisConnection* connection)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::RedisWorker %i", boost::this_thread::get_id());
    _connection = connection;
    _queue = newQueue;
    _cancelationToken = false;
    m_connected = false;
    
    _workerThread = boost::thread(&RedisWorker::WorkerThread, this);
}

RedisWorker::~RedisWorker()
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::~RedisWorker %i", boost::this_thread::get_id());

    _cancelationToken = true;

    for (std::size_t i = 0; i < io_services_.size(); ++i)
        io_services_[i]->stop();

    if (_queue)
        _queue->Cancel();

    delete m_client;
    delete m_aclient;

    _clientThread.join();
    _workerThread.join();
}

void RedisWorker::onAsyncConnect(bool connected, const std::string &errorMessage)
{
    if (connected)
    {
        m_connected = true;
        if (!_connection->m_connectionInfo.password.empty())
            m_aclient->command(0, "AUTH", _connection->m_connectionInfo.password, [&](const RedisValue &v, uint64 guid) {});
        m_aclient->command(0, "SELECT", _connection->m_connectionInfo.database, [&](const RedisValue &v, uint64 guid) {});
        //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onAsyncConnect connected Succes %i", boost::this_thread::get_id());
    }
    //else
        //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onAsyncConnect connected Faile %i", boost::this_thread::get_id());
    _connection->Unlock();
}

void RedisWorker::onGet(const RedisValue &value)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onGet value %s", value.toString().c_str());
    _connection->Unlock();
}

void RedisWorker::onSet(const RedisValue &value)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onSet value %s", value.toString().c_str());
    _connection->Unlock();
}

const RedisValue RedisWorker::GetKey(const char* cmd, const char* key)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::GetKey key %s %i", key, boost::this_thread::get_id());

    const RedisValue v = m_client->command(cmd, key);
    _connection->Unlock();
    return v;
}

const RedisValue RedisWorker::SetKey(const char* cmd, const char* key, const char* value)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::SetKey key %s value %s %i", key, value, boost::this_thread::get_id());

    const RedisValue v = m_client->command(cmd, key, value);
    _connection->Unlock();
    return v;
}

const RedisValue RedisWorker::GetKeyH(const char* cmd, const char* key, const char* field)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::GetKey key %s %i", key, boost::this_thread::get_id());

    const RedisValue v = m_client->command(cmd, key, field);
    _connection->Unlock();
    return v;
}

const RedisValue RedisWorker::SetKeyH(const char* cmd, const char* key, const char* field, const char* value)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::SetKey key %s value %s %i", key, value, boost::this_thread::get_id());

    const RedisValue v = m_client->command(cmd, key, field, value);
    _connection->Unlock();
    return v;
}

void RedisWorker::GetKeyAsync(const char* cmd, const char* key, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_REDIS, "RedisWorker::GetKey cmd %s key %s %i", cmd, key, boost::this_thread::get_id());

    m_aclient->command(guid, cmd, key, handler);
    _connection->Unlock();
}

void RedisWorker::SetKeyAsync(const char* cmd, const char* key, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::SetKey cmd %s key %s value %s guid %i thread %i", cmd, key, value, guid, boost::this_thread::get_id());

    m_aclient->command(guid, cmd, key, value, handler);
    _connection->Unlock();
}

void RedisWorker::HGetKeyAsync(const char* cmd, const char* key, const char* field, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_REDIS, "RedisWorker::GetKey cmd %s key %s %i", cmd, key, boost::this_thread::get_id());

    m_aclient->command(guid, cmd, key, field, handler);
    _connection->Unlock();
}

void RedisWorker::HSetKeyAsync(const char* cmd, const char* key, const char* field, const char* value, uint64 guid, const boost::function<void(const RedisValue &, uint64)> &handler)
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::SetKey cmd %s key %s value %s guid %i thread %i", cmd, key, value, guid, boost::this_thread::get_id());

    m_aclient->command(guid, cmd, key, field, value, handler);
    _connection->Unlock();
}

boost::asio::io_service& RedisWorker::get_io_service()
{
    // Use a round-robin scheme to choose the next io_service to use.
    boost::asio::io_service& io_service = *io_services_[next_io_service_];
    ++next_io_service_;
    if (next_io_service_ == io_services_.size())
        next_io_service_ = 0;
    return io_service;
}

void RedisWorker::Reconnect()
{
    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::Reconnect start %i", boost::this_thread::get_id());

    if (!_connection->LockIfReady()) //Try lock thread to wait recconected
        return;

    if (_queue)
    {
        m_aclient = new RedisAsyncClient(*io_services_[0]);
        m_aclient->asyncConnect(*m_endpoint[0], boost::bind(&RedisWorker::onAsyncConnect, this, _1, _2));
    }
    else
    {
        std::string errmsg;
        m_client = new RedisSyncClient(*io_services_[0]);
        m_connected = m_client->connect(*m_endpoint[0], errmsg);
        if (m_connected)
        {
            if (!_connection->m_connectionInfo.password.empty())
                m_client->command("AUTH", _connection->m_connectionInfo.password);
            m_client->command("SELECT", _connection->m_connectionInfo.database);
        }

        //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::Reconnect sync cooect %i", m_connected);
        _connection->Unlock();
    }
}

void RedisWorker::WorkerThread()
{
    next_io_service_ = 0;
    boost::asio::ip::address address = boost::asio::ip::address::from_string(_connection->m_connectionInfo.host);
    const unsigned int port = std::stoi(_connection->m_connectionInfo.port_or_socket);

    io_service_ptr io_service(new boost::asio::io_service);
    work_ptr work(new boost::asio::io_service::work(*io_service));
    endpoint_ptr endpoint(new boost::asio::ip::tcp::endpoint(address, port));

    io_services_.push_back(io_service);
    work_.push_back(work);
    m_endpoint.push_back(endpoint);

    if (_queue)
    {
        _connection->LockIfReady();
        m_aclient = new RedisAsyncClient(*io_service);
        m_aclient->asyncConnect(*endpoint, boost::bind(&RedisWorker::onAsyncConnect, this, _1, _2));
    }
    else
    {
        _connection->LockIfReady();
        std::string errmsg;
        m_client = new RedisSyncClient(*io_service);
        m_connected = m_client->connect(*endpoint, errmsg);
        if (m_connected)
        {
            if (!_connection->m_connectionInfo.password.empty())
                m_client->command("AUTH", _connection->m_connectionInfo.password);
            m_client->command("SELECT", _connection->m_connectionInfo.database);
        }
        _connection->Unlock();
    }
    _clientThread = boost::thread(boost::bind(&boost::asio::io_service::run, io_services_[0]));

    //sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::WorkerThread() run %i", boost::this_thread::get_id());

    if (!_queue)
        return;

    for (;;)
    {
        RedisOperation* operation = nullptr;

        _queue->WaitAndPop(operation);

        if (_cancelationToken || !operation)
            return;

        operation->SetConnection(_connection);
        operation->call();

        delete operation;
    }
}
