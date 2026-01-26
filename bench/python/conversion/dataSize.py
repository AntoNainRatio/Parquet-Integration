import os
import math

def total_size(main_data_tables, additional_data_tables=None):
    """
    Get the total size of the data in Ko

    Args:
        main_data_table (str): path to the main data table
        additional_data_tables (dict, default=None): dictionary containing the others data tables
    Returns:
        (int): the size in Kilo bytes
    """
    total = os.path.getsize(main_data_tables)

    if additional_data_tables != None:
        for _,val in additional_data_tables.items():
            total += os.path.getsize(val)

    return math.floor(total / 1000)