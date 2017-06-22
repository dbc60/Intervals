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

#define INTERVAL_PERIOD         resolution(10000000)
#define JITTER_MIN              100000
#define JITTER_MAX              1000000
#define ITERATION_MAX           400
#define RUNTIME_LIMIT    resolution(4000000000)


class TimeDurations {
    std::vector<duration> jitters_;
    duration smallest_;
    duration largest_;

public:
    TimeDurations()
        : jitters_(ITERATION_MAX)
        , smallest_(resolution::max())
        , largest_(resolution::min()) {
    }

    void
        insert(duration j) {
        jitters_.push_back(j);

        if (j < smallest_) {
            smallest_ = j;
        }

        if (j > largest_) {
            largest_ = j;
        }
    }

    duration average() {
        duration result(0);
        for (auto j : jitters_) {
            result = result + j;
        }

        result /= jitters_.size();

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
        std::sort(jitters_.begin(), jitters_.end());
        return jitters_[jitters_.size() / 2];
    }
};


template <int IntervalMin, int IntervalMax>
class PeriodicTimer {
private:
    volatile bool is_running_ = false;
    std::future<int> pending_;
    my_clock::time_point firstInterval_;
    my_clock::time_point lastInterval_;

    //! @brief call doIt until stop() is called, and return the number of
    // iterations for which doIt was called.
    int doItTimed(std::function<void(duration)> doIt) {
        TimeDurations durations;
        int result = 0;
        std::random_device seedGenerator;
        std::mt19937 gen(seedGenerator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);
        resolution jitter = resolution(distribution(gen));
        firstInterval_ = my_clock::now();
        my_clock::time_point start{firstInterval_};
        my_clock::time_point startNext{firstInterval_};

        while (is_running_) {
            // Delay the function call for a small random amount of time.
            std::this_thread::sleep_for(jitter);
            doIt(jitter);
            ++result;

            // Calculate the next time we want to iterate
            startNext = start + INTERVAL_PERIOD;
            auto currentTime = my_clock::now();
            durations.insert(currentTime - start);
            std::chrono::steady_clock::time_point jitter_time = start + jitter;
            std::chrono::duration<double> time_span_jitter
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    jitter_time - start);
            std::chrono::duration<double> time_span_actual
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    currentTime - start);
            std::chrono::duration<double> time_span_expected
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    startNext - start);

            /* Some days it takes just over 10 ms to run doIt(), so it helps
            to see what's happening. As it is, doIt() regularly takes over 3 ms
            on my old 2.4 GHz core i5 desktop, running Windows 10. That seems
            excessive.
            */
            if (currentTime < startNext) {
                // Wait for the next period start time
                std::this_thread::sleep_until(startNext);
            } else {
                std::cout << "Warning: Missed interval " << result
                    << ". Jitter: " << jitter.count() << ". Jitter duration: "
                    << time_span_jitter.count() << ". Duration: "
                    << time_span_actual.count() << ". Interval Limit: "
                    << time_span_expected.count() << std::endl;
            }

            // Get a new jitter and start time for the next iteration
            jitter = resolution(distribution(gen));
            start = startNext;
        }

        lastInterval_ = startNext;
        std::cout << "Shortest interval is  "
            << std::chrono::duration_cast<microsec>(durations.smallest()).count()
            << " us" << std::endl;
        std::cout << "Longest interval is   "
            << std::chrono::duration_cast<microsec>(durations.largest()).count()
            << " us" << std::endl;
        std::cout << "Average interval is   "
            << std::chrono::duration_cast<microsec>(durations.average()).count()
            << " us" << std::endl;
        std::cout << "Median interval is:   "
            << std::chrono::duration_cast<microsec>(durations.median()).count()
            << " us" << std::endl << std::endl;

        return result;
    }

public:
    //! @brief call doIt for repeat_count iterations. 
    // A random delay (jitter) is calculated for each call to doIt. If doIt
    // runs for less than the delay, doItCount will wait for the remaining time
    // before the next interval. If doIt takes more time than the delay, the
    // next iteration takes place immediately. This way, doIt is called no more
    // often than
    void doItCount(std::function<void(resolution)> doIt,
                        uint32_t repeat_count) {
        TimeDurations durations;
        std::random_device seedGenerator;
        std::mt19937 gen(seedGenerator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);

        resolution jitter = resolution(distribution(gen));
        firstInterval_ = my_clock::now();
        my_clock::time_point start{firstInterval_};
        my_clock::time_point startNext{firstInterval_};

        uint32_t itr = 0;
        while (itr < repeat_count) {
            // Delay the function call for a small random amount of time.
            std::this_thread::sleep_for(jitter);
            doIt(jitter);
            ++itr;

            // Collect some stats
            startNext = start + INTERVAL_PERIOD;
            auto currentTime = my_clock::now();
            durations.insert(currentTime - start);
            std::chrono::steady_clock::time_point jitter_time = start + jitter;
            std::chrono::duration<double> time_span_jitter
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    jitter_time - start);
            std::chrono::duration<double> time_span_actual
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    currentTime - start);
            std::chrono::duration<double> time_span_expected
                = std::chrono::duration_cast<std::chrono::duration<double>>(
                    startNext - start);

            if (currentTime < startNext) {
                // Wait for the next period start time
                std::this_thread::sleep_until(startNext);
            } else {
                std::cout << "Warning: Missed interval " << itr
                    << ". Jitter: " << jitter.count() << ". Jitter duration: "
                    << time_span_jitter.count() << ". Duration: "
                    << time_span_actual.count() << ". Interval Limit: "
                    << time_span_expected.count() << std::endl;
            }

            // Get a new jitter for the next iteration
            jitter = resolution(distribution(gen));
            start = startNext;
        }

        lastInterval_ = my_clock::now();
        std::cout << "Shortest interval is  "
            << std::chrono::duration_cast<microsec>(durations.smallest()).count()
            << " us" << std::endl;
        std::cout << "Longest interval is   "
            << std::chrono::duration_cast<microsec>(durations.largest()).count()
            << " us" << std::endl;
        std::cout << "Average interval is   "
            << std::chrono::duration_cast<microsec>(durations.average()).count()
            << " us" << std::endl;
        std::cout << "Median interval is:   "
            << std::chrono::duration_cast<microsec>(durations.median()).count()
            << " us" << std::endl << std::endl;
    }

    void start(std::function<void(resolution)> doIt) {
        is_running_ = true;
        // Run doItTimed on another thread, passing the "this" pointer and the
        // function doItTimed must run until stop() is executed.
        auto f = std::async(std::launch::async,
                            &PeriodicTimer::doItTimed,
                            this,
                            doIt);
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
        return lastInterval_ - firstInterval_;
    }
};


void main() {
    TimeDurations jitter1;
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
    timer.doItCount(std::bind(&TimeDurations::insert, &jitter1, std::placeholders::_1), ITERATION_MAX);
    auto runtime = timer.runtime();

    std::cout << "Iterations            " << ITERATION_MAX << std::endl;
    std::cout << "Expected elapsed time "
        << (ITERATION_MAX * std::chrono::duration_cast<millisec>(repeatInterval).count())
        << " ms" << std::endl;
    std::cout << "Actual elapsed time   "
        << std::chrono::duration_cast<millisec>(runtime).count()
        << " ms" << std::endl;
    std::cout << "Smallest jitter is    "
        << std::chrono::duration_cast<microsec>(jitter1.smallest()).count()
        << " us" << std::endl;
    std::cout << "Largest jitter is     "
        << std::chrono::duration_cast<microsec>(jitter1.largest()).count()
        << " us" << std::endl;
    std::cout << "Average jitter is     "
        << std::chrono::duration_cast<microsec>(jitter1.average()).count()
        << " us" << std::endl;
    std::cout << "Median jitter is      "
        << std::chrono::duration_cast<microsec>(jitter1.median()).count()
        << " us" << std::endl;

    std::cout << std::endl;

    /*** Timed Test ***/
    TimeDurations jitter2;
    const resolution iterationTimeLimit = RUNTIME_LIMIT;
    std::cout << "Jitter test 2. Timed : "
        << std::chrono::duration_cast<millisec>(iterationTimeLimit).count() << " ms" << std::endl;

    timer.start(std::bind(&TimeDurations::insert, &jitter2, std::placeholders::_1));
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
        << std::chrono::duration_cast<microsec>(jitter2.smallest()).count()
        << " us" << std::endl;
    std::cout << "Largest jitter is   "
        << std::chrono::duration_cast<microsec>(jitter2.largest()).count()
        << " us" << std::endl;
    std::cout << "Average jitter is   "
        << std::chrono::duration_cast<microsec>(jitter2.average()).count()
        << " us" << std::endl;
    std::cout << "Median jitter is:   "
        << std::chrono::duration_cast<microsec>(jitter2.median()).count()
        << " us" << std::endl;
}

