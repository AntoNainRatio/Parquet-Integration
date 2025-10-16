from khiops import core as kh
from ctypes import cdll, c_char_p, c_int
from ctypes import cdll
import os
import shutil

def extract_name(table_path):
    path = os.path.join(table_path)

    splitted = os.path.split(path)
    splitted_ext = os.path.splitext(splitted[1])
    return splitted_ext[0]

def get_multifile_path(temp_dir: str, prefix: str):
    return f"multifile://{temp_dir.replace(':','').replace('\\','/')}/{prefix}.txt"

def convert_parquet_dictionary(parquet_dictionary, temp_dir, verbose=False):
    """"
    Convert a parquet dictionary to the converted dictionary into csv
    
    Args:
        parquet_dictionary (dict): path to the parquet file to convert into csv
        temp_dir_b (str): path to the directory in which files are saved
        verbose (bool, default=False): indicates if info during execution or not
    Returns:
        dict: resulting dictionary with path of the converted data_tables as value
    """
    if parquet_dictionary == None:
        return None
    result = {}
    for key, val in parquet_dictionary.items():
        data_table_b = val.encode('utf-8')
        filename = extract_name(val)
        prefix_b = filename.encode('utf-8')
        info = parquet_utils.parquetToCsv(data_table_b, prefix_b, temp_dir.encode('utf-8'))
        if info != 0:
            return None
        result[key] = get_multifile_path(temp_dir,filename)
    return result

if __name__ == "__main__":

    parquet_utils_dll_path = os.path.join(
        os.path.dirname(__file__),
        "libs",
        "libparquet_utils.dll"
    )

    if not os.path.exists(parquet_utils_dll_path):
        raise FileNotFoundError(f"DLL non trouvée à l’emplacement : {parquet_utils_dll_path}")

    parquet_utils = cdll.LoadLibrary(parquet_utils_dll_path)


    # Définir le prototype de la fonction parquetToCsv
    parquet_utils.parquetToCsv.argtypes = [c_char_p, c_char_p, c_char_p]
    parquet_utils.parquetToCsv.restype = c_int

    #########################################################

    dict = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Accidents.kdic"
    dict_name = "Accident"
    target_variable = "Gravity"

    
    temp_dir = os.path.join(
        os.path.dirname(__file__),
        "tmp")
    
    #########################################################################################
    #                                   T_DATA                                              #
    #########################################################################################

    
    data_table = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Accidents.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_T_data")
    
    vehicles_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Vehicles.txt"
    users_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Users_medium.txt"
    places_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Places.txt"
    additional_data_tables={
        "Accident`Vehicles": vehicles_data_file,
        "Accident`Vehicles`Users":  users_data_file,
        "Accident`Place": places_data_file,
    }

    kh.train_predictor(dict,
                       dict_name,
                       data_table,
                       target_variable,
                       results_dir,
                       additional_data_tables=additional_data_tables
                       )
    
    #########################################################################################
    #                               T_DATA ( MULTIFILE )                                    #
    #########################################################################################

    
    data_table = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Accidents.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_T_data_multi")
    
    vehicles_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Vehicles.txt"
    users_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Users_medium.txt"
    places_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/cd_dm_train/T_files/T_Places.txt"
    additional_data_tables={
        "Accident`Vehicles": vehicles_data_file,
        "Accident`Vehicles`Users":  users_data_file,
        "Accident`Place": places_data_file,
    }

    kh.train_predictor(dict,
                       dict_name,
                       data_table,
                       target_variable,
                       results_dir,
                       additional_data_tables=additional_data_tables
                       )
    
    #########################################################################################
    #                     CSV  (PARQUET MERGED) - MULTIFILE DRIVER                          #
    #########################################################################################

    
    data_table = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Accidents_merged.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_csv_parquet_merged_multi")
    
    vehicles_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Vehicles_merged.txt"
    users_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Users_medium_merged.txt"
    places_data_file = "multifile://C/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Places_merged.txt"
    additional_data_tables={
        "Accident`Vehicles": vehicles_data_file,
        "Accident`Vehicles`Users":  users_data_file,
        "Accident`Place": places_data_file,
    }

    kh.train_predictor(dict,
                       dict_name,
                       data_table,
                       target_variable,
                       results_dir,
                       additional_data_tables=additional_data_tables
                       )
    
    #########################################################################################
    #                          CSV  (PARQUET MERGED) - NO DRIVER                            #
    #########################################################################################

    
    data_table = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Accidents_merged.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_csv_parquet_merged")
    
    vehicles_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Vehicles_merged.txt"
    users_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Users_medium_merged.txt"
    places_data_file = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/tmp/Places_merged.txt"
    additional_data_tables={
        "Accident`Vehicles": vehicles_data_file,
        "Accident`Vehicles`Users":  users_data_file,
        "Accident`Place": places_data_file,
    }

    kh.train_predictor(dict,
                       dict_name,
                       data_table,
                       target_variable,
                       results_dir,
                       additional_data_tables=additional_data_tables
                       )
    
    