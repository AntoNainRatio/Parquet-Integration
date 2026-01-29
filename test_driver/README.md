# Test Driver

This  folder contains tests of the multifile driver for Khiops.

## scenario tests

The scenario tests are located in each the folder present in this directory.
To test them, you just need to launch the **Shell Khiops** and run the scenario file located in each folder.

For example, to run the test , you just need to run the following command:
```
khiops -i path/to/scenario._kh
```

If you want to write the logs in a file, you can use the following command:

```
khiops -i path/to/scenario._kh -e path/to/logfile.txt
```

You can create driver by using the Khiops application and finding the khiops_data/lastrun/scenario._kh in your user folder (It was my case).

## Python tests

The python tests uses the Khiops Python API to test the multifile driver.
If you want to run them, you will need to change the paths of the files used in the tests since they are using absolute paths.

Then you simply need to run the files with Python.