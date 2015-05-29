
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class WorkQueue
{
public:
    WorkQueue(int max_queue_size = -1) : _max_queue_size(max_queue_size) { }

    bool getNextJob(T &job)
    {
        std::unique_lock<std::mutex> lock(_queue_mutex);
            
        while (true)
        {
            if (_job_queue.empty())
            {
                // no jobs in queue, wait till a new job is posted

                _job_posted.wait(lock);

                // a new jobs was posted, but someone grabbed it before us
                if (_job_queue.empty())
                {
                    return false;
                }
                // jobs's ours
                break;
            }
            else 
            {
                // jobs in queue
                break;
            }
        }

        job = _job_queue.front();

        _job_queue.pop_front();

        lock.unlock();

        _job_consumed.notify_all();

        return true;
    }

    bool postJob(T &job)
    {
        std::unique_lock<std::mutex> lock(_queue_mutex);

        while (_max_queue_size > 0 && 
               _job_queue.size() >= (unsigned)_max_queue_size)
        {
            // queue is full, wait until a job is consumed
//            printf("WorkQueue::postJob() queue is full, waiting...\n");
            _job_consumed.wait(lock);
        }

        _job_queue.push_back(job);
        
        unsigned queue_size = _job_queue.size();
        lock.unlock();
        
        if (queue_size > 1)
            _job_posted.notify_all();
        else
            _job_posted.notify_one();

        return true;
    }

    void wakeAllConsumers()
    {
        _job_posted.notify_all();
    }
        
    
    void sync()
    {
        bool done = false;
        while (!done)
        {
            std::unique_lock<std::mutex> lock(_queue_mutex);

            if (!_job_queue.empty())
            {
                _job_consumed.wait(lock);
            }

            if (_job_queue.empty())
            {
                done = true;
            }
        }
    }

    int queueSize()
    {
        _queue_mutex.lock();
        unsigned queue_size = _job_queue.size();
        _queue_mutex.unlock();
        return queue_size;
    }

private:
    int                      _max_queue_size;
    std::mutex               _queue_mutex;
    std::condition_variable  _job_posted;
    std::condition_variable  _job_consumed;
    std::deque<T>            _job_queue;
};
