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

We saw on the benchmark that threads write multiple times for files that have more batch than thread available. In this case, a thread write a batch and once it's done, it continues by reading an other etc...
This comportment lead to produce more intermediate files than there are threads available.
The more files we got, the slower is the merge.

4. **Multi-threaded Implementation with Multiple Producers (number of intermediate files = number of threads available)** (MultiSplitByNBatch.cpp)

To get some better results we can tell how many batch we want the thread to write. This way we can minimize the number of files written.
But we still have to find the number of batches that a thread writes and there is no optimal number for all the file size.

To minimize the number of intermediate files written in the most optimal way, we want to equally split the number of batches between the writing threads.
All of the threads should write between (number\_of\_batches / number\_of\_threads) - 1 and (number\_of\_batches / number\_of\_threads) + 1.

We just needed to find the number of batches in the Parquet files. But as we told before, batches are structure created when you read the file. So, it implies to read all the file to find the number of batches.
We knew that the row group structure was close to the batch structure and the row group is a native structure of Parquet files. In the metadata, we can easily find the number of row groups in the file.

5. **Multi-threaded Implementation with Multiple Producers working with row groups** (MultiSplitByNRowGroup.cpp)

We didn't change the algorithm from the previous version. We just switch to row groups reading instead of batches.
The merge was still slowing the process down. Without it, the execution time is really effective.

We thought about considering the intermediate files as a multifile. Khiops is able to read multifile on Clouds, so we know that this multifile structure will work with Khiops.
There isn't any driver that reads multifile locally.
If we create a multifile driver reading locally, Khiops will be able to use CSV multifiles comming from Parquet files. 