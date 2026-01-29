# Multifile Driver

The **multifile driver** is a driver implemented that comes with a conversion from Parquet to CSV. You can found the conversion work [here](https://github.com/AntoNainRatio/Parquet-Integration/tree/main/parquet_utils).
Those both parts creates an external integration of Parquet into **Khiops**
Khiops was able to read multifile on Clouds drivers but not locally. This version reads multifile locally.

## Implementation Process

The base used to implement this driver was the ["Null Driver"](https://github.com/KhiopsML/khiops/tree/dev/src/Norm/khiopsdriver_file_null). This driver is used as a template of an empty driver doing nothing.

The algorithm used is pretty simple and you will understand it pretty easily with the structure and the comments provided in the code.

**Khiops** verify the scheme and the path of the file. It seems that **Khiops** considered that if there's more than one ':' character, it's an invalid path.
To bypass this, we didn't put the ':' after the Hard Drive letter. For example, instead of writing `multifile://C:/path/to/file`, we write `multifile://C/path/to/file`.
This way, **Khiops** will consider the path as valid and in the driver , we will add back the ':' after the Hard Drive letter.

The Khiops team is aware of this issue and I told me to bypass it for the POC.

## How to use it

To use it with Khiops, you have to compile the code by creating a dynamic library. Once the library is produced, you willl place the files `libkhiopsdriver_file_multifile.dll`, `libkhiopsdriver_file_multifile.lib` and `libkhiopsdriver_file_multifile.exp` the `KHIOPS_DRIVER_PATH` folder.
This folder is defined by an environment variable.

Once all those steps are properly done, you will see your driver with the command:

```
khiops -s
```

This command should be typed in the **Shell Khiops**.

If you see your driver, you just need to use the **scheme** when using Khiops. I tested the driver by using **scenarios**.

## Tests

You can find some tests in the `tests` files. There's multiple things tested each with a different executables.