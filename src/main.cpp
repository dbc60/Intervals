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

#define INTERVAL_PERIOD         resolution(20000000)
#define JITTER_MIN              0
#define JITTER_MAX              100
#define ITERATION_MAX           100
#define RUNTIME_LIMIT    resolution(4000000000)


template <int IntervalMin, int IntervalMax>
class PeriodicTimer {
private:
    volatile bool is_running_ = false;
    std::future<int> pending_;
    my_clock::time_point start_;
    my_clock::time_point end_;

    //! @brief call doIt until stop() is called, and return the number of
    // iterations for which doIt was called.
    int doItTimed(std::function<void(resolution)> doIt) {
        int result = 0;
        std::random_device seedGenerator;
        std::mt19937 gen(seedGenerator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);
        resolution jitterNext = resolution(distribution(gen));
        resolution jitter;
        start_ = my_clock::now();
        my_clock::time_point start{start_};
        my_clock::time_point startNext{start};

        while (is_running_) {
            start = my_clock::now();
            my_clock::duration wakeupError = start - startNext;
            jitter = jitterNext;

            // Delay the function call for a small random amount of time.
            std::this_thread::sleep_for(jitter);
            doIt(jitter);
            ++result;

            // Get a new jitter for the next iteration
            jitterNext = resolution(distribution(gen));

            // Calculate the next time we want to iterate
            startNext = start + INTERVAL_PERIOD;

            // Wait for the next period start time
            std::this_thread::sleep_until(startNext);
        }
        end_ = my_clock::now();
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
        std::random_device seedGenerator;
        std::mt19937 gen(seedGenerator());
        std::uniform_int_distribution<> distribution(IntervalMin,
                                                     IntervalMax);

        resolution jitterNext = resolution(distribution(gen));
        resolution jitter;
        start_ = my_clock::now();
        my_clock::time_point start{start_};
        my_clock::time_point startNext{start};

        uint32_t itr = 0;
        while (itr < repeat_count) {
            auto start = my_clock::now();
            jitter = jitterNext;

            // Delay the function call for a small random amount of time.
            std::this_thread::sleep_for(jitter);
            doIt(jitter);
            ++itr;

            // Get a new jitter for the next iteration
            jitterNext = resolution(distribution(gen));

            // Calculate the next time we want to iterate
            startNext = start + INTERVAL_PERIOD;

            // Wait for the next period start time
            std::this_thread::sleep_until(startNext);
        }

        end_ = my_clock::now();
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
        // Wait until the thread is done
        std::future<int> f = std::move(pending_);

        // Return
        return f.get();
    }

    my_clock::duration runtime() const {
        return end_ - start_;
    }
};


class Jitters {
    std::vector<resolution> jitters_;
    resolution smallest_;
    resolution largest_;

public:
    Jitters()
        : smallest_(resolution::max())
        , largest_(resolution::min()) {
    }

    void
    insert(resolution j) {
        jitters_.push_back(j);

        if (j < smallest_) {
            smallest_ = j;
        }

        if (j > largest_) {
            largest_ = j;
        }
    }

    resolution::rep
    average() {
        resolution result(0);
        for (auto j : jitters_) {
            result = result + j;
        }

        result /= jitters_.size();

        return result.count();
    }

    resolution::rep
    largest() {
        return largest_.count();
    }

    resolution::rep
    smallest() {
        return smallest_.count();
    }

    resolution::rep
    median() {
        std::sort(jitters_.begin(), jitters_.end());
        return jitters_[jitters_.size() / 2].count();
    }
};


void main() {
    Jitters jitter1;
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
        << "  Min Jitter: " << jitterMin.count() << " ns" << std::endl
        << "  Max Jitter: " << jitterMax.count() << " ns" << std::endl << std::endl;

    /*** Iteration Test ***/
    std::cout << "Jitter test 1. Iterations: " << ITERATION_MAX << std::endl;
    // Now returns a std::chrono::time_point for the steady_clock. The next
    // three variables are equivalent definitions of the same type.
    auto start = my_clock::now();
    my_clock::time_point start_steady = start;
    std::chrono::time_point<my_clock> start_time_point = start;

    timer.doItCount(std::bind(&Jitters::insert, &jitter1, std::placeholders::_1), ITERATION_MAX);
    auto end = my_clock::now();

    // The elapsed time is a std::chrono::time_point
    auto elapsedTime = end - start;

    std::cout << "Iterations            " << ITERATION_MAX << std::endl;
    std::cout << "Expected elapsed time "
        << ITERATION_MAX * std::chrono::duration_cast<millisec>(repeatInterval).count()
        << " ms" << std::endl;
    std::cout << "Actual elapsed time   "
        << std::chrono::duration_cast<millisec>(elapsedTime).count()
        << " ms" << std::endl;
    std::cout << "Duration              "
        << std::chrono::duration_cast<millisec>(timer.runtime()).count()
        << " ms" << std::endl;
    std::cout << "Smallest jitter is    " << jitter1.smallest() << " ns" << std::endl;
    std::cout << "Largest jitter is     " << jitter1.largest()  << " ns" << std::endl;
    std::cout << "Average jitter is     " << jitter1.average()  << " ns" << std::endl;
    std::cout << "Median jitter is      " << jitter1.median()   << " ns" << std::endl;

    std::cout << std::endl;

    /*** Timed Test ***/
    Jitters jitter2;
    const resolution iterationTimeLimit = RUNTIME_LIMIT;
    std::cout << "Jitter test 2. Timed : "
        << std::chrono::duration_cast<millisec>(iterationTimeLimit).count() << " ms" << std::endl;

    start = my_clock::now();
    timer.start(std::bind(&Jitters::insert, &jitter2, std::placeholders::_1));
    std::this_thread::sleep_for(iterationTimeLimit);
    int iterations = timer.stop();
    end = std::chrono::steady_clock::now();
    elapsedTime = end - start;

    std::cout << "Expected iterations " << iterationTimeLimit / repeatInterval
        << std::endl;
    std::cout << "Actual iterations   " << iterations << std::endl;
    std::cout << "Elapsed time        "
        << std::chrono::duration_cast<millisec>(elapsedTime).count()
        << " ms" << std::endl;
    std::cout << "Duration            "
        << std::chrono::duration_cast<millisec>(timer.runtime()).count()
        << " ms" << std::endl;
    std::cout << "Smallest jitter is  " << jitter2.smallest() << " ns" << std::endl;
    std::cout << "Largest jitter is   " << jitter2.largest()  << " ns" << std::endl;
    std::cout << "Average jitter is   " << jitter2.average()  << " ns" << std::endl;
    std::cout << "Median jitter is:   " << jitter2.median()   << " ns" << std::endl;
}

