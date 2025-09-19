import os
import time
from khiops import core as kh
from khiops.sklearn import KhiopsClassifier

import parquetToCsv
import dir_converter

def simple_check_database_parquet(dictionary_file_path, dictionary_name, data_table_path, additional_data_tables=None, remove_csv=True):
    """
    Very naive wrapper that converts a parquet to csv and call check_database
    
    Args:
        dictionary_file_path (str): path to the file containing the dictionary(s)
        dictionary_name (str): name of the main dictionary
        data_table_path (str): path of the main data table
        additional_data_tables (dict, default=None): dictionary containing the data paths and file paths for a multi-table dictionary file
        remove_csv (boolean; default=True): deleting the csv file or not
    """

    data_table_path_converted, additional_data_tables_converted = dir_converter.convert(data_table_path,additional_data_tables)

    kh.api.check_database(dictionary_file_path_or_domain=dictionary_file_path,
                          dictionary_name=dictionary_name,
                          data_table_path=data_table_path_converted,
                          additional_data_tables=additional_data_tables_converted
                        )
    
    if remove_csv and os.path.exists(data_table_path_converted):
        os.remove(data_table_path_converted)
    if remove_csv and additional_data_tables_converted != None:
        for _, val in additional_data_tables_converted.items():
            if os.path.exists(val):
                os.remove(val)



def simple_deploy_model_parquet(dictionary_file_path, dictionary_name, data_table_path, output_data_table_path, additional_data_tables=None, remove_csv=True):
    """
    Very naive wrapper that converts a parquet to csv and call check_database producing an output file

    Args:
        path_to_dir (str): path to directory of the file
        filename_without_ext (str): filename without extension
        remove_csv (boolean; default=True): deleting the csv file or not
    """

    data_table_path_converted, additional_data_tables_converted = dir_converter.convert(data_table_path,additional_data_tables)

    kh.api.deploy_model(dictionary_file_path_or_domain=dictionary_file_path,
                          dictionary_name=dictionary_name,
                          data_table_path=data_table_path_converted,
                          output_data_table_path=output_data_table_path,
                          additional_data_tables=additional_data_tables_converted
                        )

    if remove_csv and os.path.exists(data_table_path_converted):
        os.remove(data_table_path_converted)
    if remove_csv and additional_data_tables_converted != None:
        for _, val in additional_data_tables_converted.items():
            if os.path.exists(val):
                os.remove(val)

def simple_train_predictor_parquet(dictionary_file_path, dictionary_name, data_table_path, target_variable, results_dir, additional_data_tables=None, remove_csv=True):

    data_table_path_converted, additional_data_tables_converted = dir_converter.convert(data_table_path,additional_data_tables)

    kh.api.train_predictor(dictionary_file_path,
                           dictionary_name,
                           data_table_path_converted,
                           target_variable,
                           results_dir,
                           additional_data_tables=additional_data_tables_converted
                           )

    if remove_csv and os.path.exists(data_table_path_converted):
        os.remove(data_table_path_converted)
    if remove_csv and additional_data_tables_converted != None:
        for _, val in additional_data_tables_converted.items():
            if os.path.exists(val):
                os.remove(val)