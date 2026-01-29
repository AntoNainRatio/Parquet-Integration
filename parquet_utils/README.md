# Parquet Utils

Parquet Utils is a dynamic library that is able to convert Parquet file into CSV.

It can read or write files from local or google cloud storage.

Made as external tool for [Khiops](https://github.com/KhiopsML/khiops).


## Prerequisites
### Google Cloud Storage Installation & Authentication

Make sure you have the [gcloud CLI](https://cloud.google.com/sdk/docs/install) CLI installed and configured on your machine.

To authenticate with Google Cloud Storage, you need to set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable to point to your service account key file (see [Application Default Credentials](https://cloud.google.com/docs/authentication/provide-credentials-adc)).

You can get those credentials by running the following command:

```bash
gcloud init
gcloud auth application-default login
```

### CMake
You need to install [CMake](https://cmake.org/). 

## How to build parquet_utils

Then run the following command:
```bash
cmake -B build -S . -D CMAKE_BUILD_TYPE=Release
cmake --build build --target parquet_utils
```

## How to use it

You can check `src/main.cp` for an example of usage.

## Implementation Details

The library is teh result after the benchmark done previously. We figure out that the most optimal way to convert a Parquet file into a CSV one, was to convert it into a CSV multifile.
This way, we can read and write in parallel the data, making the conversion way faster.

During the implementation, we figure out that the quoting has a impact on the results when using Khiops.
The report of model training was showing differences with and without quoting. One was not necessarily better than the other, but the results were different.
To make sure that the POC was a success, we need the same exact reports from Parquet and from CSV.

The library uses Apache Arrow to read and write Parquet and CSV files and the `arrow::csv::QuotingStyle::Needed` quote every non-numeric value.
This is considered the way it should be by Arrow. You can find an issue about it [here](https://github.com/apache/arrow/issues/42032).
I implemented a worker that quote only if it's truly needed, meaning if the value contains the delimiter, the quote character, a new line, etc...
This worker is really slow but show that the results are the same from Parquet and CSV.

This undefined behavior from Khiops has been reported [here](https://github.com/KhiopsML/khiops/issues/805).

This POC was working but needed disk space to write the multifile CSV before computing it with Khiops.
That's why we implemented a backend for this conversion. It's able to stream the data from or to multiple sources (local disk, GCS, S3, etc...).
For the moment, only GCS and local disk are implemented. But adding a new source is really easy.
We just need to implement the `RandomAccessFile` and `OutputStream` interfaces from Arrow for the new source.
You can find examples of implementation in `src/gcs_stream\*` and `src/io_backend.\*`.

Examples of usage can be found in `src/main.cp`.