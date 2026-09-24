#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <memory>

struct reference_waiter
{
    void wait_until_zero()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] { return m_count == 0; });
    }

    int thread_count()
    {
        return m_count;
    }

    struct reference_waiter_holder
    {
        reference_waiter_holder(reference_waiter& s) : m_s(s)
        {
            add();
        }

        ~reference_waiter_holder()
        {
            subtract();
        }

        reference_waiter_holder() = delete;
        reference_waiter_holder(const reference_waiter_holder&) = delete;
        const reference_waiter_holder& operator=(const reference_waiter_holder&) = delete;

        reference_waiter_holder(reference_waiter_holder&& other) noexcept : m_s(other.m_s)
        {
            add();
        }
        reference_waiter_holder& operator=(reference_waiter_holder&&) noexcept
        {
            add();
            return *this;
        }

    private:
        void add()
        {
            std::unique_lock<std::mutex> lock(m_s.m_mutex);
            m_s.m_count++;
            m_s.m_cv.notify_all(); // notify the waiting thread
        }

        void subtract()
        {
            std::unique_lock<std::mutex> lock(m_s.m_mutex);
            m_s.m_count--;
            m_s.m_cv.notify_all(); // notify the waiting thread
        }

        reference_waiter& m_s;
    };

    [[nodiscard]] reference_waiter_holder take_reference()
    {
        return reference_waiter_holder(*this);
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count = 0;
};