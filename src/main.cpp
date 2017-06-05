#include <cstdint>
#include <chrono>
#include <thread>
#include <functional>
#include <random>
#include <iostream>


#define INTERVAL_US_MIN 100
#define INTERVAL_US_MAX 500
#define COUNT_REPEAT    1000

const auto interval_min = std::chrono::microseconds(INTERVAL_US_MIN);
const auto interval_max = std::chrono::microseconds(INTERVAL_US_MAX);

//! @brief call do_something for repeat_count iterations.
void
doItRepeatedly(std::function<void(std::chrono::microseconds)> doIt,
               uint32_t repeat_count) {
    std::random_device seedGenerator;
    std::mt19937 gen(seedGenerator());
    std::uniform_int_distribution<> distribution(INTERVAL_US_MIN,
                                                 INTERVAL_US_MAX);

    std::chrono::microseconds jitter = std::chrono::microseconds(distribution(gen));

    for (uint32_t itr = 0; itr < repeat_count; ++itr) {
        auto start = std::chrono::steady_clock::now();
        doIt(jitter);
        auto end = std::chrono::steady_clock::now();
        auto elapsedTime = end - start;

        jitter = std::chrono::microseconds(distribution(gen));
        auto timeToWait = jitter - elapsedTime;
        if (timeToWait > std::chrono::microseconds::zero()) {
            std::this_thread::sleep_for(timeToWait);
        }
    }
}

class Jitters {
    std::vector<std::chrono::microseconds> jitters_;
    std::chrono::microseconds smallest_;
    std::chrono::microseconds largest_;

public:
    Jitters()
        : smallest_(std::chrono::microseconds::max())
        , largest_(std::chrono::microseconds::min()) {
    }

    void
    insert(std::chrono::microseconds j) {
        jitters_.push_back(j);

        if (j < smallest_) {
            smallest_ = j;
        }

        if (j > largest_) {
            largest_ = j;
        }
    }

    std::chrono::microseconds::rep
    average() {
        std::chrono::microseconds result(0);
        for (auto j : jitters_) {
            result = result + j;
        }

        result /= jitters_.size();

        return result.count();
    }

    std::chrono::microseconds::rep
    largest() {
        return largest_.count();
    }

    std::chrono::microseconds::rep
    smallest() {
        return smallest_.count();
    }

    std::chrono::microseconds::rep
    median() {
        return jitters_[jitters_.size() / 2].count();
    }
};


void main() {
    Jitters jitters;

    doItRepeatedly(std::bind(&Jitters::insert, &jitters, std::placeholders::_1), COUNT_REPEAT);

    std::cout << "Average jitter is " << jitters.average() << std::endl;
    std::cout << "Smallest jitter is " << jitters.smallest() << std::endl;
    std::cout << "Largest jitter is " << jitters.largest() << std::endl;
    std::cout << "Median jitter is " << jitters.median() << std::endl;
}
