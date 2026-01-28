# Python Conversion

This directory contains Python scripts that helped me during this process of integrating Parquet files with Khiops.

It contains multiple scripts that has their own purpose. Some are used to generate datasets, others to benchmark the different methods.
You can find more details about each script in the function's description.

The main scripts are:

## bench.py

It's a script that benchmarks the different methods of reading and conversion into CSV + khiops uses. You can find more details in the script's description.
It uses :
- `comparator.py` to time the differents functions exectution.
- `readers.py` to read the datasets using different methods.
- `khiops_wrapper.py` to call Khiops from Python. It has been written to take Parquet files by converting it before hand.
- `parquetToCsv.py` to convert Parquet files into CSV files.
- `dir_converter.py` to convert Parquet datasets into CSV datasets.
- `dataSize.py` to get the size of the datasets. It has been written to get the size of multitables without getting the size of each files one by one.

This benchmark gave some results but I quickly realized that the C++ implementation was way more efficient. 
It was a great way to learn how to use Khiops and find informations on Parquet files.

## csvToParquet.py

It's a script that converts CSV files into Parquet files. It has been written to generate Parquet datasets from existing CSV datasets.
It's pretty slow because it's verifying before hand if the types are correct. Once a schema is found, it uses it to convert the rest of the dataset.
I used it a lot to generate Parquet datasets and think it's working well.

## heavierMaker.py

It's a script that adds useless users to the `Accidents` dataset to make it heavier. It adds random data (or same user everytime in the last commit) to the dataset but to keep the coherence of the data, it should duplicate existing users.
I used it to be closer to the size of files of people's use.