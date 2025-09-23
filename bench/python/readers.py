import pandas as pd
import pyarrow.parquet as pq

def parquet_memory_naive_pandas(file_path):
    """
    Reading a parquet file by reading the whole file
    using pandas

    Args:
        file_path (str): path of the parquet file
    Returns:
        Tuple: number of rows and columns
    """
    parquet_file = pd.read_parquet(file_path)
    return parquet_file.shape

def parquet_memory_naive_pyarrow(file_path):
    """
    Reading a parquet file by reading the whole file and storing it in memory
    using pyarrow.parquet

    Args:
        file_path (str): file_path of the parquet
    Returns:
        Tuple: number of rows and columns
    """
    parquet_file = pq.read_table(file_path)
    return (parquet_file.num_rows,parquet_file.num_columns)


def csv_memory_naive_pandas(file_path):
    """
    Reading a csv file by reading the whole file
    using pandas

    Args:
        file_path (str): path of the parquet file
    Returns:
        Tuple: number of rows and columns
    """
    csvFile = pd.read_csv(file_path, low_memory=False)
    return csvFile.shape

def parquet_pyarrow(file_path):
    """
    Reading a parquet file by reading it row_group by row_group
    using pyarrow.parquet
    
    Args:
        file_path (str): path of the parquet file
    Returns:
        Tuple: number of columns and rows
    """
    n_cols = 0
    n_rows = 0
    parquet_file = pq.ParquetFile(file_path)
    
    for rg in range(parquet_file.num_row_groups):
        row_group = parquet_file.read_row_group(rg, use_threads=True)
        if n_cols == 0:
            n_cols = row_group.num_columns
        n_rows += row_group.num_rows

    return n_rows,n_cols

def parquet_pyarrow_mt(main_table, additional_data_tables):
    """
    Reading the parquet main table and its additional tables
    
    Returns:
        (int): the number of value read
    """
    total = 0
    rows, cols = parquet_pyarrow(main_table)
    total += cols * rows
    if additional_data_tables != None:
        for _,val in additional_data_tables.items():
            rows, cols = parquet_pyarrow(val)
            total += cols * rows
    return total

def csv_pandas(file_path):
    """
    Function simply reading a csv file using pandas

    Args:
        file_path (str): path of the csv file
        sep (str, default = ','): separator of the csv file
    Returns:
        Tuple: number of rows and columns
    """
    n_rows = 0
    n_cols = 0
    for table in pd.read_csv(file_path, chunksize=10000, sep='\t'):
        n_rows += table.shape[0]
        if n_cols == 0:
            n_cols = table.shape[1]
    return (n_rows,n_cols)

def csv_pandas_mt(main_table, additional_data_tables):
    """
    Reading the csv main table and its additional tables
    
    Returns:
        (int): the number of value read
    """
    rows, cols = csv_pandas(main_table)
    total = rows * cols

    if additional_data_tables != None:
        for _,val in additional_data_tables.items():
            rows, cols = csv_pandas(val)
            total += rows * cols
    
    return total

def read_file_debug(csv_file):
    print("Read_file_debug output:")
    with open(csv_file, "r") as f:
        print(f.readline())
    print("End of output read_file_debug")