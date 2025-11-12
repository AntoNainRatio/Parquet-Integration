# Parquet Integration

This is the work I have done during an internship in Orange.

The goal is to make **Khiops** able to read Parquet files which is a compressed file format.
The way we choose to do it, was to convert a Parquet file into a CSV one before giving it to Khiops who already know how to read CSV files.

## Here are the work:

* **bench**  Benchmark done to see what would be the best way to convert Parquet into CSV
* **khiopsdriver_multifile** Khiops driver that reads multifile
* **parquet_utils** Tools needed linked to Parquet that contains: 
    * *parquetToCsv*  Conversion
    * *merge_csv*  Merging multifile into a proper file like everyone know
* **khiopsdriver_parquet** Deprecated Khiops driver that was the predecessor of *parquet_utils* and *khiopsdriver_multifile*

LHUILLERY Antonin