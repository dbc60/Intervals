This is an example of how to run a function at regular intervals, but more than a simple loop with calls to `std::this_thread::sleep_for(some_time);`. It compensates for the time used by the called function.

I thought it would be a good idea to include a random jitter to adjust the actual interval by a small amount. In a large, distributed environment, we might have thousands of clients attempting to connect to a server, or sending a heartbeat signal to that server (to let the server know the client is still online). The network and server will probably function better if those clients don't all send their packets simultaneously. I've heard of such things happening, and it makes devops sad.

Here's some sample output:

``` sh
E:\> Intervals.exe
Smallest jitter is: 100
Largest jitter is:  500
Average jitter is:  302
Median jitter is:   343

E:\> Intervals.exe
Smallest jitter is: 100
Largest jitter is:  499
Average jitter is:  294
Median jitter is:   167

E:\> Intervals.exe
Smallest jitter is: 100
Largest jitter is:  500
Average jitter is:  299
Median jitter is:   448

E:\> Intervals.exe
Smallest jitter is: 100
Largest jitter is:  500
Average jitter is:  301
Median jitter is:   399

E:\> Intervals.exe
Smallest jitter is: 101
Largest jitter is:  500
Average jitter is:  308
Median jitter is:   336
```
