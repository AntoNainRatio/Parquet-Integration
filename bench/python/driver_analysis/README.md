# Driver Analysis

This directory contains benchmarking scripts and data for analyzing the performance of various **Khiops** drivers. 

The benchmarks focus on measuring execution time and number of calls made to the drivers during data processing tasks.

## How it works

### How to run the benchmarks
The main benchmarking script is `driver_analysis.py`, which takes a file input.
You can call it like this:
```bash
python driver_analysis.py <log_file>
```

The script will read the specified log file, which contains records of driver calls and their execution times.
It will then analyze the data and produce a summary of the performance metrics for each driver.
The log files should be formatted with each line containing the driver name, execution time, and number of calls, separated by commas. For example:

```
driver_call_name1: x
driver_call_name2: y
driver_call_name1: z
driver_call_name3: w
```

x,y,z and w are the execution time of each call in millisecondes.
You can find an example log file `driver_call_timed.txt` in this directory.

### How to generate log files

To generate log files for benchmarking, you can modify the Khiops driver code to log the necessary information during execution.
Just need to time the execution of the call and printing it to stdout or a file using the correct format.

If you use stdout, you will see the logs directly in the console when running Khiops. You can simply copy-paste them into a text file to create your log file.

## Output

The output of the benchmarking script will be a summary of the performance metrics for each driver call, including in order:

- Number of calls
- Total execution time
- Total execution time as a percentage of the overall time
- Average execution time per call
- variance of execution time per call
- Standard deviation of execution time per call
- Minimum and maximum execution time

The summary will be printed to the console for easy viewing.
You can find an example of the output in the `results.txt` file in this directory.