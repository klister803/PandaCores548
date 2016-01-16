/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
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

#include "DatabaseEnv.h"
#include "RedisWorker.h"
#include "RedisOperation.h"
#include "RedisConnection.h"
#include "ProducerConsumerQueue.h"

RedisWorker::RedisWorker(ProducerConsumerQueue<RedisOperation*>* newQueue, RedisConnection* connection)
{
    _connection = connection;
    _queue = newQueue;
    _cancelationToken = false;
    
    _workerThread = boost::thread(&RedisWorker::WorkerThread, this);
}

RedisWorker::~RedisWorker()
{
    _cancelationToken = true;

    m_ioService->stop();

    _queue->Cancel();

    _workerThread.join();
}

void RedisWorker::onConnect(bool connected, const std::string &errorMessage)
{
    if (connected)
    {
        sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onConnect connected Succes");
    }
    else
        sLog->outInfo(LOG_FILTER_SQL_DRIVER, "RedisWorker::onConnect connected Faile");
}

void RedisWorker::WorkerThread()
{
    if (!_queue)
        return;

    m_ioService = io_service_ptr(new boost::asio::io_service);
    m_work = work_ptr(new boost::asio::io_service::work(*m_ioService));

    m_ioService->run();

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
