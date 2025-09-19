import pyarrow.parquet as pq
import pyarrow as pa
import time
import os

from khiops import core as kh
from pyarrow import csv as csv

def parquet_to_csv_pandas(parquet_file, csv_file, verbose=False):
    """
    Convert a large parquet file to CSV by processing it in row_groups.
    
    Args:
        parquet_file (str): Path to the input parquet file
        csv_file (str): Path to the output CSV file
        verbose (boolean): indicates if user wants infos on what's happening
    """
    start = time.time()
    
    parquet_file = pq.ParquetFile(parquet_file)
    
    table = parquet_file.read_row_group(0, use_threads=True)

    df_chunk = table.to_pandas()
    if verbose:
      print(f"Processing row_group 0")
      print(table)  

    # Write to CSV first chunk with header
    df_chunk.to_csv(csv_file, 
                    mode='w', 
                    header=True, 
                    sep='\t',
                    index=False)
        
    
    for rg in range(1,parquet_file.num_row_groups):
        table = parquet_file.read_row_group(rg,use_threads=True)

        df_chunk = table.to_pandas(integer_object_nulls=True)
        if verbose:
            print(f"Processing row_group {rg}")
            print(table.schema)

        
        # Write to CSV by appending without header
        df_chunk.to_csv(csv_file, 
                       mode='a', 
                       header=False,
                        sep='\t',
                       index=False)
        
        # Free memory
        del df_chunk


def parquet_to_csv1(parquet_file, csv_file, verbose=False):
    """
    Convert a large parquet file to CSV by processing it in row_groups.
    
    Args:
        parquet_file (str): Path to the input parquet file
        csv_file (str): Path to the output CSV file
        verbose (boolean): indicates if user wants infos on what's happening
    """
    start = time.time()
    
    parquet_file = pq.ParquetFile(parquet_file)
    
    table = parquet_file.read_row_group(0, use_threads=True)
    
    write_options = csv.WriteOptions(include_header=True)
    csv.write_csv(table, csv_file, write_options)
    
    write_options = csv.WriteOptions(include_header=False)
    for rg in range(1,parquet_file.num_row_groups):
        table = parquet_file.read_row_group(rg,use_threads=True)

        csv.write_csv(table, csv_file, write_options)

    end = time.time()
    if verbose:
        print(f"Conversion complete. CSV file saved to {csv_file} in {end - start:.2f} secs")

def parquet_to_csv(parquet_file, csv_file, verbose=False):
    """
    Convert a large parquet file to CSV by processing it in row_groups,
    writing incrementally with PyArrow.
    """
    start = time.time()

    parquet_file = pq.ParquetFile(parquet_file)

    # --- Premier row_group avec header ---
    table = parquet_file.read_row_group(0, use_threads=True)
    write_options = csv.WriteOptions(include_header=True)

    with pa.OSFile(csv_file, "wb") as sink:   # "wb" = write (truncate)
        csv.write_csv(table, sink, write_options)

    # --- Suivants row_groups sans header ---
    write_options = csv.WriteOptions(include_header=False)

    for rg in range(1, parquet_file.num_row_groups):
        table = parquet_file.read_row_group(rg, use_threads=True)

        with pa.OSFile(csv_file, "ab") as sink:  # "ab" = append
            csv.write_csv(table, sink, write_options)
    
        # if verbose:
        #     print(f"Processed row_group {rg}")

    end = time.time()
    if verbose:
        print(f"Conversion complete. CSV file saved to {csv_file} in {(end - start)*1000:.0f} millisecs")



if __name__ == "__main__":
    parquet_file = os.path.join(kh.get_samples_dir(),"AccidentsVeryHeavy","Users_veryheavy.parquet")
    csv_file = os.path.join(kh.get_samples_dir(),"AccidentsVeryHeavy","Users_veryheavy_python.txt")
    
    parquet_to_csv(parquet_file, csv_file,verbose=True)