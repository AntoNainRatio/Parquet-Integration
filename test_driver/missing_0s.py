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


def trainpredictor_parquet(dict, dict_name, data_table, target_variable, additional_data_tables,
                           results_dir, temp_dir):
    data_table_b = data_table.encode('utf-8')
    filename = extract_name(data_table)
    prefix_b = filename.encode('utf-8')  # préfixe des fichiers de sortie
    temp_dir_b = temp_dir.encode('utf-8')

    if not os.path.exists(temp_dir):
        os.makedirs(temp_dir, exist_ok=True)

    result = parquet_utils.parquetToCsv(data_table_b, prefix_b, temp_dir_b)

    if result != 0:
        print(f"Erreur lors de la conversion du fichier {data_table_b}")
        return result
    
    convert_data = get_multifile_path(temp_dir, filename)
    convert_additional = convert_parquet_dictionary(additional_data_tables, temp_dir)

    kh.train_predictor(dict, dict_name, convert_data, target_variable, results_dir, additional_data_tables=convert_additional)

    # shutil.rmtree(temp_dir_b, ignore_errors=False, onerror=None)

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

    #################################################################################################
    #                                       MISSING 0s                                              #
    #################################################################################################

    data_table = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Accidents_output.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_missing_csv")
    
    vehicles_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Vehicles_output.txt"
    users_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium_output.txt"
    places_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Places_output.txt"
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