#include <mutex>
#include <condition_variable>

#include "MapUpdater.h"
#include "Map.h"

/// Constructor
MapUpdaterTask::MapUpdaterTask(MapUpdater *updater) : _updater(updater)
{

}

/// Notify that the task is done
void MapUpdaterTask::UpdateFinished()
{
    if (_updater != nullptr)
        _updater->update_finished();
}


class MapUpdateRequest : MapUpdaterTask
{
    private:
        Map &_map;
        uint32 _diff;

    public:
        MapUpdateRequest(Map &map, MapUpdater &updater, uint32 diff) :
            MapUpdaterTask(&updater), _map(map), _diff(diff)
        {
        }

        void call() override
        {
            uint32 diff = getMSTime();
            _map.Update(_diff);
            diff = getMSTime() - diff;
            if (diff > 750)
                sLog->outDiff("Map diff: %u. ID %i Players online: %u.", diff, _map.GetId(), sWorld->GetActiveSessionCount());
            UpdateFinished();
        }
};

void MapUpdater::activate(size_t num_threads)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        _workerThreads.push_back(std::thread(&MapUpdater::WorkerThread, this));
    }
}

void MapUpdater::deactivate()
{
    _cancelationToken = true;

    wait();

    _queue.Cancel();

    for (auto &thread : _workerThreads)
    {
        thread.join();
    }
}

void MapUpdater::wait()
{
    std::unique_lock<std::mutex> lock(_lock);

    while (_pending_requests > 0)
        _condition.wait(lock);

    lock.unlock();
}

void MapUpdater::schedule_update(Map &map, uint32 diff)
{
    std::lock_guard<std::mutex> lock(_lock);
    ++_pending_requests;
    _queue.Push((MapUpdaterTask*)new MapUpdateRequest(map, *this, diff));
}

void MapUpdater::schedule_specific(MapUpdaterTask *request)
{
    std::lock_guard<std::mutex> lock(_lock);
    ++_pending_requests;
    _queue.Push(request);
}

bool MapUpdater::activated()
{
    return _workerThreads.size() > 0;
}

void MapUpdater::update_finished()
{
    std::lock_guard<std::mutex> lock(_lock);
    --_pending_requests;
    _condition.notify_all();
}

void MapUpdater::WorkerThread()
{
    while (true)
    {
        MapUpdaterTask *request = nullptr;

        _queue.WaitAndPop(request);

        if (_cancelationToken)
            return;

        request->call();
        delete request;
    }
}