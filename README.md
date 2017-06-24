# Intervals and Jitter

The code here is mostly an exercise to help me learn how to use some of the newer features of C++11 and templates in the C++ standard library. There are two functions, `doItCounted` and `doItTimed` that accept a `std::function` and run it. The first one runs the function a given number of times. It's fairly straight forward.

The second runs a given function at regular intervals via a call to `start(std::function)` and will continue until `stop()` is called. The nice thing about `doItTimed` is that it compensates for the time used by the called function. As long as the function completes within an interval, `doItTimed` will start it at regular intervals with a slight amount of jitter added to the start time.

I thought it would be a good idea to include a random jitter to adjust when the called function is started, because in a distributed environment we might have thousands of clients attempting to connect to a server, or sending a heartbeat signal to that server (to let the server know the client is still online). The network and server will probably function better if those clients don't all send their packets simultaneously. I've heard of such things happening, and it makes devops sad.

The code uses the following functions and templates from the standard library:

- `steady_clock`, `system_clock` and `high_resolution_clock` from `std::chrono`
- `std::chrono::time_point`
- `std::chrono::duration` and `std::chrono::duration_cast`
- `std::this_thread`
- `std::async`
- `std::bind`
- `std::move`
- `std::future`
- `std::random_device`
- `std::mt19937`
- `std::uniform_int_distribution`
- `std::vector`
- `std::ostream` (`std::cout` and related I/O functions)

Here is some sample output:

``` sh
E:\> Debug\Intervals.exe
The resolution of the high-resolution clock is: 1e-09 sec
The resolution of the steady clock is:          1e-09 sec
The resolution of the system clock is:          1e-07 sec

Jitter Tests.
  Interval:   10 ms
  Min Jitter: 100 us
  Max Jitter: 1000 us

Jitter test 1. Iterations: 400
Missed intervals 0
Shortest interval is  4697 ns
Longest interval is   41843 ns
Average interval is   5339 ns
Median interval is:   4697 ns

Iterations            400
Expected elapsed time 4000 ms
Actual elapsed time   4000 ms
Smallest jitter is    100 us
Largest jitter is     996 us
Average jitter is     279 us
Median jitter is      100 us

Jitter test 2. Timed : 4000 ms
Missed intervals 0
Shortest interval is  4269 ns
Longest interval is   96922 ns
Average interval is   5663 ns
Median interval is:   4269 ns

Expected iterations 401
Actual iterations   401
Elapsed time        4010 ms
Smallest jitter is  100 us
Largest jitter is   999 us
Average jitter is   269 us
Median jitter is:   100 us

E:\> Release\Intervals.exe
The resolution of the high-resolution clock is: 1e-09 sec
The resolution of the steady clock is:          1e-09 sec
The resolution of the system clock is:          1e-07 sec

Jitter Tests.
  Interval:   10 ms
  Min Jitter: 100 us
  Max Jitter: 1000 us

Jitter test 1. Iterations: 400
Missed intervals 0
Shortest interval is  0 ns
Longest interval is   13663 ns
Average interval is   251 ns
Median interval is:   0 ns

Iterations            400
Expected elapsed time 4000 ms
Actual elapsed time   4000 ms
Smallest jitter is    103 us
Largest jitter is     998 us
Average jitter is     274 us
Median jitter is      103 us

Jitter test 2. Timed : 4000 ms
Missed intervals 0
Shortest interval is  0 ns
Longest interval is   53798 ns
Average interval is   356 ns
Median interval is:   0 ns

Expected iterations 401
Actual iterations   401
Elapsed time        4010 ms
Smallest jitter is  101 us
Largest jitter is   998 us
Average jitter is   281 us
Median jitter is:   101 us```
