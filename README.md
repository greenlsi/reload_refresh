# Reload+Refresh

This is a proof of concept of the work done for "RELOAD+REFRESH: Abusing Cache Replacement Policies to Perform Stealthy Cache Attacks" 

This code was designed for Intel processors and Linux Systems.

`Warning: this is a proof-of-concept, only useful for testing the performance of RELOAD+REFRESH and compare it with previous existing approaches. Use it under your own risk.`

## Requirements

It requires Hugepages and assumes they are mounted on `/mnt/hugetlbfs/`. This value can be modified by changing the value of FILE_NAME.
Once reserved, hugepages can be mounted: `$ sudo mount -t hugetlbfs none /mnt/hugetlbfs`
Note that this may require to use `sudo` for the examples or to change the permissions of the `/mnt/hugetlbfs/` folder.

For ploting the results it requires **python3** and **matplotlib**

## Configuration

#### Architecture details

In order for the example to work properly, these values must be manually changed in the `cache_details.h` file:

```
/*Cache and memory architecture details*/
#define CACHE_SIZE 6 //MB
#define CPU_CORES 4
#define CACHE_SET_SIZE 12 //Ways
#define CACHE_SLICES 8
#define SETS_PER_SLICE 1024
#define BITS_SET 10 //log2(SETS_PER_SLICE)
#define BITS_LINE 6 //64 bytes per cache line

#define BITS_HUGEPAGE 21
#define BITS_NORMALPAGE 12
```

#### Options and calibration

Different options can be configured in various source files, `example_code.c' and 'example_code_sync.c`

```
#define TIME_LIMIT 190         /*Time for main memory access, must be calibrated*/
#define CLEAN_REFRESH_TIME 620 /*Time for Refresh, must be calibrated*/
#define TIME_PRIME 650         /*Time for Prime, must be calibrated*/
#define NUM_CANDIDATES 3 * CACHE_SET_SIZE *CACHE_SLICES
#define RES_MEM (1UL * 1024 * 1024) * 4 * CACHE_SIZE
#define CLEAN_NOISE 1 /*For Reload+Refresh to detect noise and reset cache state*/
#define WAIT_FIXED 1  /*Defines if it is possible to "wait" between samples*/
```
Note that the timings are machine dependand and as a consquence, must be calibrated. We do not include any specific utility for calibration, although the outputs of the "examples" can be used for this task. 
* NUM_CANDIDATES defines the number of adresses mapping to a particular set that will be used to create the eviction sets
* RES_MEM  size of memory that will be reserved as hugepages
* CLEAN_NOISE defines whether we want to revert the state of the cache if noise is detected (1), that is, refresh time > CLEAN_REFRESH_TIME or not (0)
* WAIT_FIXED defines if the test must consider some "waiting time" in cycles (1) between samples or not (0)

Other configuration in `cache_utils.h`

```
#define SLICE_HASH_AVAILABLE 1
```
Defines whether it is possible to use the physical addresses (1) or not (0) in that machine.

## Installation

Only a `make` is required to build the static binary.

`Warning:` It builds a shared library `libT.so` . Since it is not in an standard location of the system, one easy way to use it is:
export LD_LIBRARY_PATH=.

## How to use

It is possible to test different approaches in two different scenarios, both involve two processes victim and attacker:

### Synchronous test

* Victim: sender_sync
  * Receives requests through port 10000 and performs a timed access (depending on a value of the request) to the target address, saves access times and timestamps in a file.
* Atacker: example_code_sync
  * Sends requests through port 10000 and performs the desired attack against the target address, once the request has been recieved saves attack times and timestamps in a file.
* Plot: the results can be depicted in a graph by calling `pyhton3  py_plot_sync.py sender_file attacker_file`

#### Options

* sender_sync
  * Expects the IP address of the machine and a target address (int)
  * `$ ./sender_sync 127.0.0.1 1350`
* example_code_sync
  * Expects IP address, target address (int), attack name (-fr,-pp,-fpp,-rr) and number of samples to collect.
    * The attacks that can be tested include Flush+Reload (-fr), Prime+Probe (-pp), fast Prime+Probe (-fpp) as described in the Rowhammer.js paper and also requires the values of S,C and D and RELOAD+REFRESH (-rr)
  * `$ ./example_code_sync 127.0.0.1 1350 -pp 60000`
  
  Note that the target address must be the same in both cases.

### Spy process (no synchronized)

This test evaluates a covert channel between sender and receiver (example code)

* Victim: sender
  * Performs one access to a target address and waits the nanoseconds given as argument before the next access, repeats this procedure for NUM_SAMPLES. Once it has finished asks for a new "waiting time". The value of NUM_SAMPLES is "defined" in the code. Saves results in a file (access times and timestamps). 
* Atacker: example_code
  * Monitors the target address using the technique given as argument (-fr,-pp,-fpp,-rr) during a maximum time and outputs the results to a file. 
* Filter: the results can be filtered using prepare_file.sh sender_file attacker_file. **Warning:** This script overwrittes the attacker_file, so save a copy if you run multiple tests using the same file.
* Plot  `pyhton3 py_plot_sync.py sender_file attacker_file`

#### Options

* Victim: sender
  * Once it is running asks for a target address (int), and then asks for the time (in ns) that it must wait between accesses to the target. It keeps running and asking for times as input
  * `$ ./sender`
* Atacker: example_code
  * Expects target address (int), attack name (-fr,-pp,-rr), wait cycles between samples and maximum run time in seconds. Although if the value of WAIT_FIXED is equal to 0 the value of wait cycles is not used, the program requires it.
    * The attacks that can be tested include Flush+Reload (-fr), Prime+Probe (-pp), fast Prime+Probe (-fpp) as described in the Rowhammer.js paper and also requires the values of S,C and D and RELOAD+REFRESH (-rr)
  * `$ ./example_code 1352 -rr 12 15`

**NOTE** that the output of both senders can be used to callibrate the main memory access time, whereas the output of the attack  processes can be used to callibrate RELOAD+REFRESH or PRIME+PROBE. Similarly the number of samples of the sender with times over the main memory access threshold indicates aproximately the number of misses the victim sees.
**NOTE** In RELOAD+REFRESH attacks "low" access times indicate the victim has used the data. 


## Tested systems

This code has been successfully tested on:

* Intel(R) Core(TM) i5-7600K CPU @ 3.80GHz
	* Cores (4x1)
	* L3: 6MB
	* Associativity: 12
	* Cache sets: 8192
	* Slices: 8
* Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz
	* Cores (4x2)
	* L3: 8MB
	* Associativity: 16
	* Cache sets: 8192
	* Slices: 8
  
  ## Example figure output
