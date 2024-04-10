
#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>

namespace OPNX {
    /*
    * threadsafe queue
    * T queue data type
    * This class does not support copy constructors or assignment operators
    */
    template <typename T>
    class OrderQueue
    {
    private:
        mutable std::mutex m_mutex;
        mutable std::mutex m_mutexPop;
        mutable std::condition_variable m_condition;
        using queue_type = std::queue<T>;
        queue_type m_queueData;

    public:
        using value_type = typename queue_type::value_type;
        using container_type = typename queue_type::container_type;

        OrderQueue() = default;
        OrderQueue(const OrderQueue &) = delete;
        OrderQueue &operator=(const OrderQueue &) = delete;
        /*
        * Constructor using iterator as parameter, applicable to all container objects
        */
        template <typename _InputIterator>
        OrderQueue(_InputIterator first, _InputIterator last)
        {
            for (auto it = first; it != last; ++it)
            {
                m_queueData.push(*it);
            }
        }
        explicit OrderQueue(const container_type &c) : m_queueData(c) {}

        void push(const value_type &new_value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queueData.push(std::move(new_value));
            m_condition.notify_all();
        }

        /*
        * Pop an element from the queue and block if the queue is empty
        * */
        value_type wait_and_pop()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this]{ return !this->m_queueData.empty(); });

            auto value = std::move(m_queueData.front());
            m_queueData.pop();
            return value;
        }
        bool wait_and_pop(value_type &value, int iTimeout=1)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_condition.wait_for(lock, std::chrono::seconds(iTimeout), [this]{ return !this->m_queueData.empty(); }))
            {
                return false;
            }

            value = std::move(m_queueData.front());
            m_queueData.pop();
            return true;
        }
        /*
        * Pop an element from the queue, and return false if the queue is empty
        * */
        bool try_pop(value_type &value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queueData.empty())
                return false;
            value = std::move(m_queueData.front());
            m_queueData.pop();
            return true;
        }
        /*
        * Return whether the queue is empty
        * */
        auto empty() const -> decltype(m_queueData.empty())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queueData.empty();
        }
        /*
        * Returns the number of elements in the queue
        * */
        auto size() const -> decltype(m_queueData.size())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queueData.size();
        }
    }; /* ThreadSafeQueue */
}

#endif // THREAD_QUEUE_H
