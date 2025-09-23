import os
from khiops import core as kh
import timeit

# Our code
import comparator
import readers
import khiops_wrapper
import dataSize

def bench_file(number_of_calls, 
               path_to_dir, 
               data_table_filename_no_ext, 
               dictionary_filepath, 
               dictionary_name,
               target_variable,
               additional_data_tables_csv = None, 
               additional_data_tables_parquet = None,
               ):
    """
    Launching a benchmark on a file that got csv and parquet same file
    Benchmark contains check_database, deploy_model, train_predictor

    Args:
        number_of_calls (int): number of time functions will be launched to make a mean from
        path_to_dir (str): path to the directory containing all the tables
        data_table_filename_no_ext (str): filename of the main data table without it's extension
        dictionary_filepath (str): path of the dictionary file
        dictionary_name (str): name of the dictionary
        additional_data_tables_csv (dict): dict structure containing the additional_data_tables in csv format
        additional_data_tables_parquet (dict): dict structure containing the additional_data_tables in parquet format
    """
    full_path_csv = os.path.join(path_to_dir, data_table_filename_no_ext+'.txt')
    print(f"full_path_csv: {full_path_csv}")
    full_path_parquet = os.path.join(path_to_dir, data_table_filename_no_ext+'.parquet')

    dir_path, filename = os.path.split(full_path_csv)
    filename, ext = os.path.splitext(filename)
    output_data_table_path = os.path.join(dir_path,filename+'_output'+ext)

    # readers.read_file_debug(full_path_csv)
    total_values_read_csv = readers.csv_pandas_mt(full_path_csv, additional_data_tables_csv)
    total_values_read_parquet = readers.parquet_pyarrow_mt(full_path_parquet, additional_data_tables_parquet)

    if total_values_read_csv != total_values_read_csv:
        raise Exception(f"File error: Not same number of values read between csv and parquet files.(CSV file got {total_values_read_csv} and parquet got {total_values_read_parquet})")
    
    
    print(f"Files total: ", end='')
    print('{:,}'.format(total_values_read_csv).replace(',', ' '), end='')
    print(" values read")

    total_size_csv = dataSize.total_size(full_path_csv, additional_data_tables_csv)
    total_size_parquet = dataSize.total_size(full_path_parquet, additional_data_tables_parquet)

    print(f"Files total size in csv: ", end='')
    print('{:,}'.format(total_size_csv).replace(',', ' '), end='')
    print(" Ko")

    print(f"Files total size in parquet: ", end='')
    print('{:,}'.format(total_size_parquet).replace(',', ' '), end='')
    print(" Ko")

    comparator.time_function(readers.csv_pandas_mt, number_of_calls, full_path_csv, additional_data_tables_csv)
    comparator.time_function(readers.parquet_pyarrow_mt, number_of_calls, full_path_parquet, additional_data_tables_parquet)

    comparator.time_function(kh.api.check_database,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_csv,
                             additional_data_tables=additional_data_tables_csv)
    
    comparator.time_function(khiops_wrapper.simple_check_database_parquet,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_parquet,
                             additional_data_tables_parquet,
                             remove_csv=False)

    comparator.time_function(kh.api.deploy_model,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_csv,
                             output_data_table_path,
                             additional_data_tables=additional_data_tables_csv)
    
    comparator.time_function(khiops_wrapper.simple_deploy_model_parquet,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_parquet,
                             output_data_table_path,
                             additional_data_tables=additional_data_tables_parquet,
                             remove_csv=False,
                             )
    
    comparator.time_function(kh.api.train_predictor,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_csv,
                             target_variable,
                             dir_path,
                             additional_data_tables=additional_data_tables_csv,
                             )

    comparator.time_function(khiops_wrapper.simple_train_predictor_parquet,
                             number_of_calls,
                             dictionary_filepath, dictionary_name,
                             full_path_parquet,
                             target_variable,
                             dir_path,
                             additional_data_tables_parquet,
                             remove_csv=False,
                             )



if __name__ == "__main__":

    ########################################################
    ##################       ARGS       ####################
    ########################################################



    ########################################################
    ############      SpliceJunctionDNA       ##############
    ########################################################


    # dirname = "SpliceJunction"
    # path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    # dictionary_filepath = os.path.join(path_to_dir, "SpliceJunctionDNA.kdic")
    # dictionary_name = "SpliceJunctionDNA"
    
    # data_table_filename_no_ext = "SpliceJunctionDNA"

    # additional_data_tables_csv=None

    # additional_data_tables_parquet=None

    # target_variable = "Char"

    # number_of_calls = 10
    

    ########################################################
    ##############          Adult           ################
    ########################################################


    dirname = "Adult"
    path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    dictionary_filepath = os.path.join(path_to_dir, "Adult.kdic")
    dictionary_name = "Adult"
    
    data_table_filename_no_ext = "Adult"

    additional_data_tables_csv=None

    additional_data_tables_parquet=None

    target_variable = "class"

    number_of_calls = 10



    ########################################################
    ################       Accidents       #################
    ########################################################

    # dirname = "Accidents"
    # path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    # dictionary_filepath = os.path.join(path_to_dir, "Accidents.kdic")
    # dictionary_name = "Accident"
    
    # data_table_filename_no_ext = "Accidents"

    # places_data_file_csv = os.path.join(path_to_dir, "Places.txt")
    # places_data_file_parquet = os.path.join(path_to_dir, "Places.parquet")
    # print(f"places data table: {places_data_file_csv}")
    # print("")

    # users_data_file_csv = os.path.join(path_to_dir, "Users.txt")
    # users_data_file_parquet = os.path.join(path_to_dir, "Users.parquet")
    # print(f"Users data table: {users_data_file_csv}")
    # print("")

    # vehicles_data_file_csv = os.path.join(path_to_dir, "Vehicles.txt")
    # vehicles_data_file_parquet = os.path.join(path_to_dir, "Vehicles.parquet")
    # print(f"Vehicles data table: {vehicles_data_file_csv}")
    # print("")

    # additional_data_tables_csv={
    #     "Accident`Vehicles": vehicles_data_file_csv,
    #     "Accident`Vehicles`Users":  users_data_file_csv,
    #     "Accident`Place": places_data_file_csv,
    # }

    # additional_data_tables_parquet={
    #     "Accident`Vehicles": vehicles_data_file_parquet,
    #     "Accident`Vehicles`Users":  users_data_file_parquet,
    #     "Accident`Place": places_data_file_parquet,
    # }

    # target_variable = "Gravity"

    # number_of_calls = 5



    ########################################################
    ############       Accidents Medium       ###############
    ########################################################

    # dirname = "AccidentsMedium"
    # path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    # dictionary_filepath = os.path.join(path_to_dir, "Accidents.kdic")
    # dictionary_name = "Accident"
    
    # data_table_filename_no_ext = "Accidents"

    # places_data_file_csv = os.path.join(path_to_dir, "Places.txt")
    # places_data_file_parquet = os.path.join(path_to_dir, "Places.parquet")
    # print(f"places data table: {places_data_file_csv}")
    # print("")

    # users_data_file_csv = os.path.join(path_to_dir, "Users_medium.txt")
    # users_data_file_parquet = os.path.join(path_to_dir, "Users_medium.parquet")
    # print(f"Users data table: {users_data_file_csv}")
    # print("")

    # vehicles_data_file_csv = os.path.join(path_to_dir, "Vehicles.txt")
    # vehicles_data_file_parquet = os.path.join(path_to_dir, "Vehicles.parquet")
    # print(f"Vehicles data table: {vehicles_data_file_csv}")
    # print("")

    # additional_data_tables_csv={
    #     "Accident`Vehicles": vehicles_data_file_csv,
    #     "Accident`Vehicles`Users":  users_data_file_csv,
    #     "Accident`Place": places_data_file_csv,
    # }

    # additional_data_tables_parquet={
    #     "Accident`Vehicles": vehicles_data_file_parquet,
    #     "Accident`Vehicles`Users":  users_data_file_parquet,
    #     "Accident`Place": places_data_file_parquet,
    # }

    # target_variable = "Gravity"

    # number_of_calls = 5



    ########################################################
    ############       Accidents Heavy       ###############
    ########################################################

    # dirname = "AccidentsHeavy"
    # path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    # dictionary_filepath = os.path.join(path_to_dir, "Accidents.kdic")
    # dictionary_name = "Accident"
    
    # data_table_filename_no_ext = "Accidents"

    # places_data_file_csv = os.path.join(path_to_dir, "Places.txt")
    # places_data_file_parquet = os.path.join(path_to_dir, "Places.parquet")
    # print(f"places data table: {places_data_file_csv}")
    # print("")

    # users_data_file_csv = os.path.join(path_to_dir, "Users_heavy.txt")
    # users_data_file_parquet = os.path.join(path_to_dir, "Users_heavy.parquet")
    # print(f"Users data table: {users_data_file_csv}")
    # print("")

    # vehicles_data_file_csv = os.path.join(path_to_dir, "Vehicles.txt")
    # vehicles_data_file_parquet = os.path.join(path_to_dir, "Vehicles.parquet")
    # print(f"Vehicles data table: {vehicles_data_file_csv}")
    # print("")

    # additional_data_tables_csv={
    #     "Accident`Vehicles": vehicles_data_file_csv,
    #     "Accident`Vehicles`Users":  users_data_file_csv,
    #     "Accident`Place": places_data_file_csv,
    # }

    # additional_data_tables_parquet={
    #     "Accident`Vehicles": vehicles_data_file_parquet,
    #     "Accident`Vehicles`Users":  users_data_file_parquet,
    #     "Accident`Place": places_data_file_parquet,
    # }

    # target_variable = "Gravity"

    # number_of_calls = 5


    ########################################################
    ##########      Accidents Very Heavy       #############
    ########################################################

    dirname = "AccidentsVeryHeavy"
    path_to_dir = os.path.join(kh.get_samples_dir(),dirname)

    dictionary_filepath = os.path.join(path_to_dir, "Accidents.kdic")
    dictionary_name = "Accident"
    
    data_table_filename_no_ext = "Accidents"

    places_data_file_csv = os.path.join(path_to_dir, "Places.txt")
    places_data_file_parquet = os.path.join(path_to_dir, "Places.parquet")
    print(f"places data table: {places_data_file_csv}")
    print("")

    users_data_file_csv = os.path.join(path_to_dir, "Users_veryheavy.txt")
    users_data_file_parquet = os.path.join(path_to_dir, "Users_veryheavy.parquet")
    print(f"Users data table: {users_data_file_csv}")
    print("")

    vehicles_data_file_csv = os.path.join(path_to_dir, "Vehicles.txt")
    vehicles_data_file_parquet = os.path.join(path_to_dir, "Vehicles.parquet")
    print(f"Vehicles data table: {vehicles_data_file_csv}")
    print("")

    additional_data_tables_csv={
        "Accident`Vehicles": vehicles_data_file_csv,
        "Accident`Vehicles`Users":  users_data_file_csv,
        "Accident`Place": places_data_file_csv,
    }

    additional_data_tables_parquet={
        "Accident`Vehicles": vehicles_data_file_parquet,
        "Accident`Vehicles`Users":  users_data_file_parquet,
        "Accident`Place": places_data_file_parquet,
    }

    target_variable = "Gravity"

    number_of_calls = 1


    ########################################################
    ################     END OF ARGS        ################
    ########################################################

    bench_file(number_of_calls,
               path_to_dir,
               data_table_filename_no_ext,
               dictionary_filepath, dictionary_name,
               target_variable,
               additional_data_tables_csv, additional_data_tables_parquet)

