#pragma once

#include "../log.hpp"

#include "msequence.hpp"

#include <metricq/types.hpp>

#include <sched.h>

#include <chrono>
#include <vector>

#include <cassert>

using Clock = metricq::Clock;
using TimeValue = metricq::TimeValue;

namespace timesync
{

uint64_t sqrtsd_loop_(double* buffer, uint64_t elems, uint64_t repeat);

class Footprint
{
public:
    Footprint() : compute_vec_a_(compute_size, 1.0), compute_vec_b_(compute_size, 2.0)
    {
        Log::info() << "staring synchronization pattern";
        run();
        Log::info() << "completed synchronization pattern";
    }

    Clock::time_point time_begin() const
    {
        return time_begin_;
    }

    Clock::time_point time() const
    {
        return time_begin_ + (time_end_ - time_begin_) / 2;
    }

    Clock::time_point time_end() const
    {
        return time_end_;
    }

    const std::vector<TimeValue>& recording() const
    {
        return recording_;
    };

private:
protected:
    void low();
    void high();

    template <typename DURATION>
    auto low(DURATION duration)
    {
        auto time = Clock::now();
        auto end = time + duration;
        do
        {
            low();
            time = Clock::now();
        } while (time < end);
        recording_.emplace_back(time, 0.0);
        return time;
    }

    template <typename DURATION>
    auto high(DURATION duration)
    {
        auto time = Clock::now();
        auto end = time + duration;
        do
        {
            high();
            time = Clock::now();
        } while (time < end);
        recording_.emplace_back(time, 1.0);
        return time;
    }

    template <typename DURATION>
    auto run(bool high_low, DURATION duration)
    {
        if (high_low)
        {
            return high(duration);
        }
        else
        {
            return low(duration);
        }
    }

    void run();

    void check_affinity();
    void restore_affinity();

private:
    static constexpr std::size_t compute_size = 256;
    static constexpr std::size_t compute_rep = 58;
    static constexpr std::size_t nop_rep = 209;
    Clock::time_point time_begin_;
    Clock::time_point time_end_;

    std::vector<double> compute_vec_a_;
    std::vector<double> compute_vec_b_;
    std::vector<TimeValue> recording_;

    bool restore_affinity_ = false;
    cpu_set_t cpu_set_old_;
};
} // namespace timesync
