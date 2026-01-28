# C++ Conversion Benchmark

This directory contains benchmarks for the C++ implementation of the Parquet to CSV conversion.
The benchmarks focus on measuring the performance of the C++ code in terms of execution time and memory efficiency when converting Parquet files to CSV format.

I will considered each conversion chronologically to show the improvements made over time.

1. **Initial Implementation** (Mono.cpp)

Same implementation as the Python one but in C++. It reads a Parquet file and writes a CSV file.
Better performance than Python due to compiled nature of C++.

2. **Multi-threaded Implementation** (Multi.cpp)

Introduced multi-threading to read and write in parallel using a producer-consumer model.
One thread reads the Parquet file and pushes data to a queue, while another thread consumes data from the queue and writes it to the CSV file.
The portion of data being processed was a batch. This structure is native from Arrow and is close to row grousps since it both represents a huge portion of rows.

This conversion was not memory efficient as the queue could grow indefinitely if the writer thread was slower than the reader thread.
To mitigate this, a bounded queue was implemented in the next version.

3. **Multi-threaded Implementation with Multiple Producers** (MultiSplitByBatch.cpp)

We saw with the previous version that having a single producer thread could become a bottleneck if it was slower than the consumer thread.

As the name suggests, this version introduced multiple producer threads to read the Parquet file in parallel. This way we speed up the writing process.
Now the principal thread spawns multiple writer threads, each responsible for writing a portion of the CSV file.

Now that we had multiple producer threads, we needed to ensure that the data was written in the correct order.
To achieve this, each producer write a file with the number of the batch it processed. Once all threads are done, the main thread merges all the files in the correct order.

