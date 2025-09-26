#include <cstdint>
#include <iostream>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unistd.h>

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimeTask
{
    private:
        uint64_t _id; // task id
        uint32_t _timeout; // timeout
        TaskFunc _task_cb; // task callback function
        ReleaseFunc _release; // Used to delete timer object information stored in TimerWheel
    public:
        TimeTask(uint64_t id, uint32_t delay, const TaskFunc& cb)
            : _id(id), _timeout(delay), _task_cb(cb) 
        {

        }

        ~TimeTask() 
        {
            _task_cb();
            _release();
        }

        void SetRelease(const ReleaseFunc& cb) 
        {
            _release = cb;
        }

        uint32_t DelayTime()
        {
            return _timeout;
        }

};

class TimeWheel
{
    private:
        using WeakTask = std::weak_ptr<TimeTask>;
        using PtrTask = std::shared_ptr<TimeTask>;
        int _tick; // current tick
        int _capacity; // wheel capacity
        std::vector<std::vector<PtrTask>> _wheel; // time wheel
        std::unordered_map<uint64_t, WeakTask> _timers; // task map
    private:
        void RemoveTimer(uint64_t id)
        {
            auto it = _timers.find(id);
            if (it != _timers.end()) {
                _timers.erase(it);
            }
        }

    public:
        TimeWheel()
            : _capacity(60), _tick(0), _wheel(_capacity)
        {

        }

        // Add a timer task
        void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc& cb)
        {
            PtrTask pt(new TimeTask(id, delay, cb));
            pt->SetRelease(std::bind(RemoveTimer, this, id));
            int pos = (_tick + delay) % _capacity;
            _wheel[pos].push_back(pt);
            _timers[id] = WeakTask(pt);
        }

        // Refresh a timer task
        void TimerRefresh(uint64_t id)
        {
            // Construct a shared_ptr from the saved timer object's weak_ptr 
            // and add it to the timewheel.
            auto it = _timers.find(id);
            if (it == _timers.end()) 
            {
                // Timer not found
                return;
            }

            PtrTask pt = it->second.lock(); // Convert weak_ptr to shared_ptr
            int delay = pt->DelayTime();
            int pos = (_tick + delay) % _capacity;
            _wheel[pos].push_back(pt);
        }

        // This function executes once per second, 
        // equivalent to the second hand moving backward one step.
        void RunTimerTick()
        {
            _tick = (_tick + 1) % _capacity;
            _wheel[_tick].clear(); // Clear the tasks in the current slot
        }


};

class Test
{
    public:
        Test()
        {
            std::cout << "Test()" << std::endl;
        }
        ~Test()
        {
            std::cout << "~Test()" << std::endl;
        }
};

void DelTest(Test* t)
{
    delete t;
}

int main()
{
    TimeWheel tw;
    Test* t = new Test();

    tw.TimerAdd(888, 5, std::bind(DelTest, t));

    for (int i = 0; i < 5; ++i) {
        tw.TimerRefresh(888); // Refresh the timer task
        tw.RunTimerTick(); // Move the time wheel one step
        std::cout << "tick " << i+1 << std::endl;
        sleep(1);
    }

    while (1) {
        sleep(1);
        std::cout << "tick " << std::endl;
        tw.RunTimerTick(); // Move the time wheel one step   
    }

    return 0;
}