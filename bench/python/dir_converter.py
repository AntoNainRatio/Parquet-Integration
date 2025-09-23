from khiops import core as kh
import os
import timeit

import parquetToCsv
import comparator

def convert_parquet_file(parquet_file, verbose=False):
    """"
    Convert a parquet file to a csv file
    
    Args:
        parquet_file (str): path to the parquet file to convert into csv
        verbose (bool, default=true): indicates if info during execution or not
    Returns:
        (str): path to the resulting csv file
    """
    dir_path,file_name = os.path.split(parquet_file)
    file_name_without_ext, _ = os.path.splitext(file_name)
    # print(f"dir_path: {dir_path}")
    # print(f"file_name_without_ext: {file_name_without_ext}")
    # print(f"ext: {ext}")

    csv_file = os.path.join(dir_path,file_name_without_ext+'_converted.txt')
    parquetToCsv.parquet_to_csv(parquet_file=parquet_file, csv_file=csv_file, verbose=verbose)
    return csv_file

def convert_parquet_dictionary(parquet_dictionary, verbose=False):
    """"
    Convert a parquet dictionary to the converted dictionary into csv
    
    Args:
        parquet_dictionary (dict): path to the parquet file to convert into csv
        verbose (bool, default=False): indicates if info during execution or not
    Returns:
        dict: resulting dictionary
    """
    if parquet_dictionary == None:
        return None
    result = {}
    for key, val in parquet_dictionary.items():
        result[key] = convert_parquet_file(val, verbose=verbose)
    return result

def convert(main_data_tables, additional_data_tables):
    """
    Convert main_data_table and additional_data_tables

    Returns:
        tuple: containing the converted main_data_tables path and the converted additional_data_tables dict
    """
    main_converted = convert_parquet_file(main_data_tables)
    additional_converted = convert_parquet_dictionary(additional_data_tables)
    return main_converted, additional_converted

if __name__ == "__main__":

    accidents_dataset_dir = os.path.join(kh.get_samples_dir(), "Accidents")

    vehicles_data_file = os.path.join(accidents_dataset_dir, "Vehicles.parquet")
    # print(f"Vehicles data table: {vehicles_data_file}")
    # print("")

    users_data_file = os.path.join(accidents_dataset_dir, "Users.parquet")
    # print(f"Users data table: {users_data_file}")
    # print("")

    dirname = "AccidentsHeavy"
    path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    dictionary_filepath = os.path.join(path_to_dir, "Accidents.kdic")
    dictionary_name = "Accident"
    
    data_table_parquet = os.path.join(path_to_dir,"Accidents.parquet")

    places_data_file_parquet = os.path.join(path_to_dir, "Places.parquet")

    users_data_file_parquet = os.path.join(path_to_dir, "Users_heavy.parquet")

    vehicles_data_file_parquet = os.path.join(path_to_dir, "Vehicles.parquet")

    additional_data_tables_parquet={
        "Accident`Vehicles": vehicles_data_file_parquet,
        "Accident`Vehicles`Users":  users_data_file_parquet,
        "Accident`Place": places_data_file_parquet,
    }

    comparator.time_function(convert, 3, data_table_parquet, additional_data_tables_parquet)