#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
  SafeQueue(void)
    : q()
    , m()
    , c()
  {}

  ~SafeQueue(void)
  {}

  // Add an element to the queue.
  void enqueue(T t)
  {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
  }


  // try to enqueue without blocking (enqueue only if the mutex is not locked by another thread)
  bool try_enqueue(T t)
  {
    if (m.try_lock()) {
      q.push(t);
      m.unlock();
      c.notify_one();
      return true;
    }
    return false;
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  T dequeue(void)
  {
    std::unique_lock<std::mutex> lock(m);
    while(q.empty())
    {
      // release lock as long as the wait and reaquire it afterwards.
      c.wait(lock);
    }
    T val = q.front();
    q.pop();
    return val;
  }

  // Try to dequeue without blocking
  // Returns std::nullopt if queue is empty
  std::optional<T> try_dequeue(void)
  {
    std::lock_guard<std::mutex> lock(m);
    if (q.empty()) {
      return std::nullopt;
    }
    T val = q.front();
    q.pop();
    return val;
  }

  bool empty(void) const
  {
    std::lock_guard<std::mutex> lock(m);
    return q.empty();
  }

  // Get the size of the queue
  size_t size(void) const
  {
    std::lock_guard<std::mutex> lock(m);
    return q.size();
  }

  void clear(void)
  {
    std::lock_guard<std::mutex> lock(m);
    while (!q.empty()) {
      q.pop();
    }
  }


private:
  std::queue<T> q;
  mutable std::mutex m;
  std::condition_variable c;
};
#endif
