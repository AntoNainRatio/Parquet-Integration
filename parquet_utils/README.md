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





