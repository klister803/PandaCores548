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

#ifndef _REDISWORKER_H
#define _REDISWORKER_H

#include <thread>
#include "ProducerConsumerQueue.h"
#include "Log.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>


class RedisConnection;
class RedisOperation;

class RedisWorker
{
    public:
        typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
        typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

        RedisWorker(ProducerConsumerQueue<RedisOperation*>* newQueue, RedisConnection* connection);
        ~RedisWorker();
        void onConnect(bool connected, const std::string &errorMessage);

        work_ptr m_work;
        io_service_ptr m_ioService;

    private:
        ProducerConsumerQueue<RedisOperation*>* _queue;
        RedisConnection* _connection;

        void WorkerThread();
        boost::thread _workerThread;

        std::atomic_bool _cancelationToken;

        RedisWorker(RedisWorker const& right) = delete;
        RedisWorker& operator=(RedisWorker const& right) = delete;
};

#endif
