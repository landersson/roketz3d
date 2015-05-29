
#include "WorkQueue.h"

#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>


class ImageData
{
public:
    ImageData() = default; 
    ImageData(uint8_t *data, int wid, int id) : _data(data), _size(size), _id(id) { }

    uint8_t* data() const { return _data; }    
    int id() const { return _id; }

private:
    uint8_t* _data;
    int      _width;
    int      _height;
    int      _id;
};

static bool done = false;
static WorkQueue<ImageData> img_queue;

int jpeg_writer_thread(int id)
{
    while (!done)
    {
        ImageData img_data;

        if (img_queue.getNextJob(img_data))
        {
            printf("thread %d: got job id %d\n", id, img_data.id());
            sleep(id + 1);
        }
        
    }
}


int main()
{
    const int num_threads = 3;
    std::vector<std::thread> threads;

    for (unsigned i = 0; i < num_threads; i++)
    {
        threads.push_back(std::thread(jpeg_writer_thread, i));
    }

    for(unsigned i = 0; i < 100; i++)
    {
        ImageData data(0, 0, i);

        img_queue.postJob(data);
    }

    img_queue.sync();

    done = true;

    img_queue.wakeAllConsumers();

    for (auto &t : threads)
    {
        t.join();
    }

    return 0;
}
