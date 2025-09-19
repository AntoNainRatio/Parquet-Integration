import timeit
import readers

def time_function(function, number=10, *args, **kwargs):
    """
    Time the execution of a function

    Args:
        function (func): the function
        number (int): number of calls to make and produce a mean from
        args (*args): function's arguments
        kwargs (**kwargs): function's keyword arguments
    """

    duration_mean = timeit.timeit(
                    lambda: function(*args, **kwargs),
                    number=number,
                ) / number

    print(f"Time elapsed for {function.__name__}: ", end='')
    print(f"{duration_mean:.2f}", end = ' secs\n')



def compare_read(file_path, *functions):
    """
    Compare reading time of the file by using functions. Should make another call for differents file or file extension.

    Args:
        file_path (str): path of the file taken in your read functions
        functions (*args): the differents functions
    """

    print('-' * 50)
    print(f"Measuring reading execution on \"{file_path}\":")
    print('-' * 50+'\n')

    for function in functions:
        time_function(function, 10, file_path)



def main():
    file_path_csv = "data/small.csv"
    file_path_parquet = "data/small.parquet"

    f1 = readers.csv_memory_naive_pandas
    f2 = readers.csv_pandas

    f3 = readers.parquet_memory_naive_pandas
    f4 = readers.parquet_memory_naive_pyarrow
    f5 = readers.parquet_pyarrow

    compare_read(file_path_csv, f1, f2)
    compare_read(file_path_parquet, f3, f4, f5)


if __name__ == "__main__":
    main()