# Parquet Integration

This is the work I have done during an internship in Orange.

The goal is to make [**Khiops**](https://khiops.org/) able to read Parquet files without modifying Khiops internally.
Before going for changing Khiops itself, my task was to see if we it's possible to integrate **Parquet** externally.
The way we choose to do it, was to convert a Parquet file into a CSV one before giving it to Khiops who already know how to read CSV files.

## Prerequisites

Here are the prerequisites needed to run the code

### CMake

You need to have CMake installed on your computer to compile the C++ code.
You can find the installation instructions [here](https://cmake.org/install/).

### vcpkg

You need to have vcpkg installed on your computer to manage the C++ dependencies.
You can find the installation instructions [here](https://vcpkg.io/en/).

### Khiops

You need to have Khiops installed on your computer to use the Khiops driver.
You can find the installation instructions [here](https://khiops.org/download/).

## Here are the work:

* **bench**  All the benchmarks done to compare the performance of the different methods
* **khiopsdriver_multifile** Khiops driver that reads CSV multifiles
* **parquet_utils** Tools needed to convert Parquet files into CSV multifiles: 
    * *parquetToCsv*  Conversion able to read/write from local or cloud storage
    * *merge_csv*  Merging multifile into a proper file like everyone know
* **test_driver** Contains all the tests done to validate the different parts

Each part has its own README file explaining how to use it.

This project also contains a driver implementation that can read Parquet files directly. You can found it [here](https://github.com/AntoNainRatio/parquet_reader).
It has been kept separated because it's using two brenches that represent two different ways to read Parquet files.

## How it has been done

I used Visual Studio 2022 as IDE to write the C++ code and Visual Studio Code to write the Python code.
The C++ code is compiled using CMake and the dependencies are managed using vcpkg. You can easily decide which executables you want to compile on Visual Studio 2022.

I also used Git Bash to make my commits and push them to GitHub.

You can read my internship report [here](https://github.com/AntoNainRatio/Parquet-Integration/blob/main/2027_antonin_lhuillery_rapportdestage_fr.pdf) to get more details about the work done during this internship.
I will explain things, that are not in the report, in all the README of each directories.