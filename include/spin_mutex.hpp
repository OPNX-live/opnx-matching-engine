#ifndef CORE_INC_SPIN_MUTEX_HPP_
#define CORE_INC_SPIN_MUTEX_HPP_
#include <atomic>
#include <mutex>

namespace OPNX
{
    class spin_mutex: public std::mutex
	{
	private:
		using flag = std::atomic<unsigned long long>;
	private:
		flag _f;
	public:
		spin_mutex():
			_f(0)
		{
		}
		~spin_mutex() = default;
		spin_mutex(const spin_mutex&) = delete;
		spin_mutex(spin_mutex&&) = delete;
		spin_mutex& operator= (const spin_mutex&) = delete;
		spin_mutex& operator= (spin_mutex&&) = delete;
		void lock()
		{
			unsigned long long expected = 0;
			do
			{
				expected = 0;
			}
			while (!_f.compare_exchange_weak(expected, 1, std::memory_order_relaxed, std::memory_order_relaxed));
		}

		bool try_lock()
		{
			unsigned long long expected = 0;
			return _f.compare_exchange_weak(expected, 1, std::memory_order_relaxed, std::memory_order_relaxed);
		}

		void unlock()
		{
			_f.store(0, std::memory_order_relaxed);
		}
	};

    class CAutoMutex {
    public:
        CAutoMutex(spin_mutex& pMutex) {
            m_pMutex = &pMutex;
            m_pMutex->lock();
        }

        ~CAutoMutex() {
            m_pMutex->unlock();
        }

    private:
        spin_mutex* m_pMutex;
    };
}



#endif /* CORE_INC_SPIN_MUTEX_HPP_ */
