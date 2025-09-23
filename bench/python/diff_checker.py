# Importing difflib
import difflib
import os
from khiops import core as kh

def check_diff(file1, file2):

    with open(file1) as file_1:
        file_1_text = file_1.readlines()

    with open(file2) as file_2:
        file_2_text = file_2.readlines()

    # Find and print the diff:
    for line in difflib.unified_diff(
            file_1_text, file_2_text, fromfile=file1, 
            tofile=file2, lineterm=''):
        print(line)

if __name__ == "__main__":
    file1 = "C:/Users/Public/khiops_data/samples/AccidentsHeavy/Users_heavy_cpp.txt"
    file2 = "C:/Users/Public/khiops_data/samples/AccidentsHeavy/Users_heavy.txt"
    check_diff(file1, file2)