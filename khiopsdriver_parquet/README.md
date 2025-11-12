# khiopsdriver_parquet - DEPRECATED

Driver created for poc

### Problem of this driver is that:
* each fopen call the parquetToCsv conversion
* opening a parquet creates n csv files with the following format **chunk_{0-n}.txt**. When opening differents files, we can't distinguish which chunk comes from a specific files.

The better version consists of **parquet_utils** and **khiopsdriver_multifile** files which together do the job.