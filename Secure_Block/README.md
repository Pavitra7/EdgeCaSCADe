# Cache monitor

## Running the simulator

1. Build the simulator:
    - `mkdir build && cd build`
    - `cmake ..`
    - `make`

    The executable is located in `build/bin`

2. Generate trace file:
    - `comet.sim -f [blablabla] | grep "MEMTRACE" > [tracefile]`

3. Run the trace file through the simulator:
    - `cat [tracefile] | bin/cache_monitor.sim -a [secretAddress] -s [samplingTime] -t [threshold]`
