#ifndef _MAP_UPDATER_H_INCLUDED
#define _MAP_UPDATER_H_INCLUDED

#include <mutex>
#include <thread>
#include <condition_variable>

#include "Define.h"
#include "ProducerConsumerQueue.h"

class MapUpdater;

class MapUpdaterTask
{
    public:
        /// Constructor
        MapUpdaterTask(MapUpdater *updater);

        virtual void call() = 0;

        /// Notify that the task is done
        void UpdateFinished();

    private:
        MapUpdater* _updater;
};

class Map;

class MapUpdater
{
    public:
        MapUpdater() : _cancelationToken(false), _pending_requests(0) {}
        ~MapUpdater() {};

        friend class MapUpdaterTask;

        void update_finished();
        void schedule_update(Map &map, uint32 diff);
        void schedule_specific(MapUpdaterTask *request);

        void wait();

        void activate(size_t num_threads);
        void deactivate();

        bool activated();

    private:
        ProducerConsumerQueue<MapUpdaterTask *> _queue;

        std::vector<std::thread> _workerThreads;
        std::atomic<bool> _cancelationToken;
        std::mutex _lock;
        std::condition_variable _condition;

        size_t _pending_requests;

        void WorkerThread();
};

#endif //_MAP_UPDATER_H_INCLUDED
