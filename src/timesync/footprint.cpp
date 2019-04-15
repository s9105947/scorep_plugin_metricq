#include <chrono>
#include <vector>

#include "footprint.hpp"
#include "msequence.hpp"
#include <cassert>

namespace timesync
{
void Footprint::high()
{
    double m = 0.0;
    for (std::size_t r = 0; r < compute_rep; r++)
    {
        for (size_t i = 0; i < compute_size; i++)
        {
            m += compute_vec_a_[i] * compute_vec_b_[i];
        }
    }
    if (m == 42.0)
    {
        // prevent optimization, sure there is an easier way
        __asm__ __volatile__("mfence;" :::);
    }
}

void Footprint::low()
{
    for (uint64_t i = 0; i < nop_rep; i++)
    {
        asm volatile("rep; nop" ::: "memory");
    }
}

void Footprint::check_affinity()
{
    CPU_ZERO(&cpu_set_old_);
    auto err = sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set_old_);
    if (err)
    {
        Log::error() << "failed to get thread affinity: " << strerror(errno);
        return;
    }

    cpu_set_t cpu_set_target;
    CPU_ZERO(&cpu_set_target);
    CPU_SET(0, &cpu_set_target);
    err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set_target);
    if (err)
    {
        Log::error() << "failed to set thread affinity: " << strerror(errno);
        return;
    }
    restore_affinity_ = true;
}

void Footprint::restore_affinity()
{
    if (restore_affinity_)
    {
        auto err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set_old_);
        if (err)
        {
            Log::error() << "failed to restore thread affinity: " << strerror(errno);
        }
    }
}

void Footprint::run()
{
    check_affinity();

    recording_.resize(0);
    recording_.reserve(4096);

    constexpr int n = 6;
    constexpr auto time_quantum = std::chrono::milliseconds(32);
    auto sequence = GroupedBinaryMSequence(n);

    time_begin_ = low(std::chrono::seconds(3));
    time_end_ = time_begin_;
    auto deadline = time_begin_;
    while (auto elem = sequence.take())
    {
        auto [is_high, length] = *elem;
        auto duration = time_quantum * length;
        deadline += duration;
        assert(deadline > time_end_);
        auto wait = deadline - time_end_;
        if (is_high)
        {
            time_end_ = high(wait);
        }
        else
        {
            time_end_ = low(wait);
        }
    }

    low(std::chrono::seconds(3));

    restore_affinity();
}

} // namespace timesync
