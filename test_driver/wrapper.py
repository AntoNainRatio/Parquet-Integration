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

def get_normal_path(multifile_path):
    return f"{multifile_path.replace('multifile://c','c:')}"

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

def get_output_path(path):
    tmp = os.path.split(path)
    output_dir = "C:/Users/KXFJ3896/Documents/Parquet-Integration/test_driver/output"
    return get_normal_path(output_dir+'/'+os.path.splitext(tmp[0])[0] + '_output' + '.txt')

def get_output_dict(dict):
    res = {}
    for key,val in dict.items():
        res[key] = get_output_path(val)
    return res

def dump_dict(convert_additional):
    print(f"convert_dict:")
    for key,val in convert_additional.items():
        print(f"\tconvert_additional[{key}]: {val}")
    print(f"End of convert_additional")

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
    print(f"conver_data: {convert_data}")
    convert_additional = convert_parquet_dictionary(additional_data_tables, temp_dir)
    dump_dict(convert_additional)


    kh.train_predictor(dict, dict_name, convert_data, target_variable, results_dir, additional_data_tables=convert_additional, output_scenario_path=results_dir+'\\scenario._kh')

    # output_path = get_output_path(data_table)
    # print(f'output_path: {output_path}')
    # output_dict = get_output_dict(additional_data_tables)
    # print("Output_dict:")
    # dump_dict(output_dict)
    # kh.deploy_model(dict,dict_name,
    #                 convert_data, output_path,
    #                 additional_data_tables = convert_additional,
    #                 output_additional_data_tables = output_dict
    # )


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
    data_table = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Accidents.parquet"
    target_variable = "Gravity"

    vehicles_data_file_parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Vehicles.parquet"
    users_data_file_parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet"
    places_data_file_parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Places.parquet"
    additional_data_tables_parquet={
        "Accident`Vehicles": vehicles_data_file_parquet,
        "Accident`Vehicles`Users":  users_data_file_parquet,
        "Accident`Place": places_data_file_parquet,
    }

    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_parq")
    
    temp_dir = os.path.join(
        os.path.dirname(__file__),
        "tmp")


    trainpredictor_parquet(dict,
                           dict_name,
                           data_table,
                           target_variable,
                           additional_data_tables_parquet,
                           results_dir,
                           temp_dir)
    
    #########################################################

    data_table = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Accidents.txt"
    results_dir = os.path.join(
        os.path.dirname(__file__),
        "results_csv")
    
    vehicles_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Vehicles.txt"
    users_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.txt"
    places_data_file = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Places.txt"
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
                       additional_data_tables=additional_data_tables,
                       output_scenario_path=results_dir+'\\scenario._kh'
                       )
    
    # data_table_out = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Accidents_output_csv.txt"
    # results_dir = os.path.join(
    #     os.path.dirname(__file__),
    #     "results_csv")
    
    # vehicles_data_file_out = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Vehicles_output_csv.txt"
    # users_data_file_out = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium_output_csv.txt"
    # places_data_file_out = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Places_output_csv.txt"
    # additional_data_tables_out={
    #     "Accident`Vehicles": vehicles_data_file_out,
    #     "Accident`Vehicles`Users":  users_data_file_out,
    #     "Accident`Place": places_data_file_out,
    # }

    # kh.deploy_model(dict, dict_name,
    #                 data_table,
    #                 data_table_out,
    #                 additional_data_tables=additional_data_tables,
    #                 output_additional_data_tables=additional_data_tables_out)