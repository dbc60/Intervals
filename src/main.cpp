#include <cstdint>
#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <random>
#include <iostream>
#include <vector>
#include <algorithm>


using microsec = std::chrono::microseconds;
using millisec = std::chrono::milliseconds;
using nanosec = std::chrono::nanoseconds;
using resolution = nanosec;
using my_clock = std::chrono::steady_clock;
using duration = my_clock::duration;

// 10 ms == 10,000,000 ns
#define INTERVAL_PERIOD         resolution(10000000)

// 0.1 ms == 100,000 ns
#define JITTER_MIN              100000

// 1 ms == 1,000,000 ns
#define JITTER_MAX              1000000

// Maximum number of iterations to run doItCounted
#define ITERATION_MAX           400

// 4 s = 4,000,000,000 ns
#define RUNTIME_LIMIT    resolution(4000000000)


class TimeDurations {
    std::vector<duration> event_duration_;
    duration smallest_;
    duration largest_;

public:
    TimeDurations()
        : event_duration_(ITERATION_MAX)
        , smallest_(resolution::max())
        , largest_(resolution::min()) {
    }

    void
        insert(duration ed) {
        event_duration_.push_back(ed);

        if (ed < smallest_) {
            smallest_ = ed;
        }

        if (ed > largest_) {
            largest_ = ed;
        }
    }

    duration average() {
        duration result(0);
        for (auto ed : event_duration_) {
            result = result + ed;
        }

        result /= event_duration_.size();

        return result;
    }

    duration
        largest() {
        return largest_;
    }

    duration
        smallest() {
        return smallest_;
    }

    duration
        median() {
        std::sort(event_duration_.begin(), event_duration_.end());
        return event_duration_[event_duration_.size() / 2];
    }
};


template <int IntervalMin, int IntervalMax>
class PeriodicTimer {
private:
    volatile bool           is_running_ = false;
    std::future<int>        pending_;
    /* Record the time if the first and last intervals to calculate the total
    time in which do_it() is executed.
    */
    my_clock::time_point    interval_first_;
    my_clock::time_point    interval_last_;

    //! @brief call do_it until stop() is called, and return the number of
    // iterations for which do_it was called.
    int doItTimed(std::function<void(duration)> do_it) {
        TimeDurations durations;
        int result = 0;
        int missed_intervals = 0;
        std::random_device seed_generator;
        std::mt19937 gen(seed_generator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);
        resolution jitter = resolution(distribution(gen));
        my_clock::time_point time_current;
        my_clock::time_point time_start_do_it;

        // Set the time of the first interval
        interval_first_ = my_clock::now();
        my_clock::time_point interval_current_start{interval_first_};
        my_clock::time_point interval_next_start{interval_current_start + INTERVAL_PERIOD};
        my_clock::time_point time_do_it = interval_current_start + jitter;

        while (is_running_) {
            time_current = my_clock::now();
            if (time_current < time_do_it) {
                // Wait for the next interval + jitter
                std::this_thread::sleep_until(time_do_it);
            } else {
                // Count the interval as missed and run do_it immediately
                ++missed_intervals;
            }

            // Get current time to more accurately measure do_it()'s duration 
            time_start_do_it = my_clock::now();
            do_it(jitter);

            // Record the duration of do_it
            time_current = my_clock::now();
            durations.insert(time_current - time_start_do_it);

            // Update the iteration count
            ++result;

            // Get a new jitter for the next iteration
            jitter = resolution(distribution(gen));

            // Update the start times for the next interval
            interval_current_start = interval_next_start;
            interval_next_start += INTERVAL_PERIOD;
            time_do_it = interval_current_start + jitter;
        }

        interval_last_ = interval_current_start;
        std::cout << "Missed intervals " << missed_intervals << std::endl;
        std::cout << "Shortest interval is  "
            << durations.smallest().count() << " ns" << std::endl;
        std::cout << "Longest interval is   "
            << durations.largest().count() << " ns" << std::endl;
        std::cout << "Average interval is   "
            << durations.average().count() << " ns" << std::endl;
        std::cout << "Median interval is:   "
            << durations.median().count()  << " ns" << std::endl << std::endl;

        return result;
    }

public:
    //! @brief call do_it for repeat_count iterations. 
    // A random delay (jitter) is calculated for each call to do_it. If do_it
    // runs for less than the delay, doItCounted will wait for the remaining time
    // before the next interval. If do_it takes more time than the delay, the
    // next iteration takes place immediately. This way, do_it is called no more
    // often than
    void doItCounted(std::function<void(resolution)> do_it,
                     uint32_t repeat_count) {
        TimeDurations durations;
        int missed_intervals = 0;
        std::random_device seed_generator;
        std::mt19937 gen(seed_generator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);

        resolution jitter = resolution(distribution(gen));
        my_clock::time_point time_current;
        my_clock::time_point time_start_do_it;

        // Set the time of the first interval
        interval_first_ = my_clock::now();
        my_clock::time_point interval_current_start{interval_first_};
        my_clock::time_point interval_next_start{interval_current_start + INTERVAL_PERIOD};
        my_clock::time_point time_do_it = interval_current_start + jitter;

        uint32_t itr = 0;
        while (itr < repeat_count) {
            time_current = my_clock::now();
            if (time_current < time_do_it) {
                // Sleep until jitter ns beyond the interval
                std::this_thread::sleep_until(time_do_it);
            }

            time_start_do_it = my_clock::now();
            do_it(jitter);
            // Collect some stats
            time_current = my_clock::now();
            durations.insert(time_current - time_start_do_it);
            ++itr;

            // Get a new jitter for the next iteration
            jitter = resolution(distribution(gen));

            // Update the start times for the next interval
            interval_current_start = interval_next_start;
            interval_next_start += INTERVAL_PERIOD;
            time_do_it = interval_current_start + jitter;
        }

        interval_last_ = interval_current_start;
        std::cout << "Missed intervals " << missed_intervals << std::endl;
        std::cout << "Shortest interval is  "
            << durations.smallest().count() << " ns" << std::endl;
        std::cout << "Longest interval is   "
            << durations.largest().count() << " ns" << std::endl;
        std::cout << "Average interval is   "
            << durations.average().count() << " ns" << std::endl;
        std::cout << "Median interval is:   "
            << durations.median().count() << " ns" << std::endl << std::endl;
    }

    void interval_current_start(std::function<void(resolution)> do_it) {
        is_running_ = true;
        // Run doItTimed on another thread, passing the "this" pointer and the
        // function doItTimed must run until stop() is executed.
        auto f = std::async(std::launch::async,
                            &PeriodicTimer::doItTimed,
                            this,
                            do_it);
        // Move the future to another variable so we don't wait for it here.
        pending_ = std::move(f);
    }

    int stop() {
        // Allow doItTimed to exit its while-loop
        is_running_ = false;

        // Return the number of iterations
        return pending_.get();
    }

    my_clock::duration runtime() const {
        return interval_last_ - interval_first_;
    }
};


void main() {
    TimeDurations durations1;
    const resolution jitterMin = resolution(JITTER_MIN);
    const resolution jitterMax = resolution(JITTER_MAX);
    const resolution repeatInterval = INTERVAL_PERIOD;
    PeriodicTimer<JITTER_MIN, JITTER_MAX> timer;

    std::cout << "The resolution of the high-resolution clock is: "
        << (static_cast<double>(std::chrono::high_resolution_clock::period::num)
            / std::chrono::high_resolution_clock::period::den) << " sec"
        << std::endl;
    std::cout << "The resolution of the steady clock is:          "
        << (static_cast<double>(std::chrono::steady_clock::period::num)
            / std::chrono::steady_clock::period::den)
        << " sec" << std::endl;
    std::cout << "The resolution of the system clock is:          "
        << (static_cast<double>(std::chrono::system_clock::period::num)
            / std::chrono::system_clock::period::den)
        << " sec" << std::endl << std::endl;

    std::cout << "Jitter Tests." << std::endl
        << "  Interval:   "
        << std::chrono::duration_cast<millisec>(repeatInterval).count()
        << " ms" << std::endl
        << "  Min Jitter: "
        << std::chrono::duration_cast<microsec>(jitterMin).count() << " us"
        << std::endl
        << "  Max Jitter: "
        << std::chrono::duration_cast<microsec>(jitterMax).count() << " us"
        << std::endl << std::endl;

    /*** Iteration Test ***/
    std::cout << "Jitter test 1. Iterations: " << ITERATION_MAX << std::endl;
    timer.doItCounted(std::bind(&TimeDurations::insert, &durations1, std::placeholders::_1), ITERATION_MAX);
    resolution runtime = timer.runtime();

    std::cout << "Iterations            " << ITERATION_MAX << std::endl;
    std::cout << "Expected elapsed time "
        << (ITERATION_MAX * std::chrono::duration_cast<millisec>(repeatInterval).count())
        << " ms" << std::endl;
    std::cout << "Actual elapsed time   "
        << std::chrono::duration_cast<millisec>(runtime).count()
        << " ms" << std::endl;
    std::cout << "Smallest jitter is    "
        << std::chrono::duration_cast<microsec>(durations1.smallest()).count()
        << " us" << std::endl;
    std::cout << "Largest jitter is     "
        << std::chrono::duration_cast<microsec>(durations1.largest()).count()
        << " us" << std::endl;
    std::cout << "Average jitter is     "
        << std::chrono::duration_cast<microsec>(durations1.average()).count()
        << " us" << std::endl;
    std::cout << "Median jitter is      "
        << std::chrono::duration_cast<microsec>(durations1.median()).count()
        << " us" << std::endl;

    std::cout << std::endl;

    /*** Timed Test ***/
    TimeDurations durations2;
    const resolution iterationTimeLimit = RUNTIME_LIMIT;
    std::cout << "Jitter test 2. Timed : "
        << std::chrono::duration_cast<millisec>(iterationTimeLimit).count() << " ms" << std::endl;

    timer.interval_current_start(std::bind(&TimeDurations::insert, &durations2, std::placeholders::_1));
    std::this_thread::sleep_for(iterationTimeLimit);
    int iterations = timer.stop();
    runtime = timer.runtime();

    std::cout << "Expected iterations " << runtime / repeatInterval
        << std::endl;
    std::cout << "Actual iterations   " << iterations << std::endl;
    std::cout << "Elapsed time        "
        << std::chrono::duration_cast<millisec>(runtime).count()
        << " ms" << std::endl;
    std::cout << "Smallest jitter is  "
        << std::chrono::duration_cast<microsec>(durations2.smallest()).count()
        << " us" << std::endl;
    std::cout << "Largest jitter is   "
        << std::chrono::duration_cast<microsec>(durations2.largest()).count()
        << " us" << std::endl;
    std::cout << "Average jitter is   "
        << std::chrono::duration_cast<microsec>(durations2.average()).count()
        << " us" << std::endl;
    std::cout << "Median jitter is:   "
        << std::chrono::duration_cast<microsec>(durations2.median()).count()
        << " us" << std::endl;
}

