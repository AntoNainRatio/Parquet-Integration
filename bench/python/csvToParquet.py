import pyarrow as pa
import pyarrow.parquet as pq
import pyarrow.csv as csv
import pandas as pd
import time
import os

from khiops import core as kh


def is_equal(pq_type, pd_type):
    if pq_type == pa.string() and  pd_type.name == "string":
        return True
    if pq_type == pa.int64() and pd.api.types.is_integer_dtype(pd_type):
        return True
    if pq_type == pa.float64() and pd.api.types.is_float_dtype(pd_type):
        return True
    if pq_type == pa.bool_() and pd.api.types.is_bool_dtype(pd_type):
        return True
    if pq_type == pa.timestamp("ns") and pd.api.types.is_datetime64_any_dtype(pd_type):
        return True
    return False

def normalize_types(pq_type, pd_type, sample_series=None):
    # Cas compatibles inchangés
    if is_equal(pq_type, pd_type):
        return pq_type

    # Si la colonne est object, on teste son contenu
    if pd.api.types.is_object_dtype(pd_type) and sample_series is not None:
        numeric = pd.to_numeric(sample_series, errors="coerce")
        print(f"Numeric {numeric}")
        ratio_numeric = numeric.notna().mean()
        if ratio_numeric > 0.7:
            # Vérifier si tous les nombres sont entiers

            # if (numeric.dropna() % 1 == 0).all():
                return pa.int64()
            # else:
            #     return pa.float64()
        else:
            return pa.string()

    # Si c’est un mélange int et float → float64
    if (pq_type in [pa.int64(), pa.float64()]) and \
       (pd.api.types.is_integer_dtype(pd_type) or pd.api.types.is_float_dtype(pd_type)):
        return pa.float64()

    # Par défaut → string
    return pa.string()

def find_schema(csv_file, chunksize=10000, sep="\t", verbose=False):
    """
    Trouve un schéma qui marche pour tout le fichier en lisant par chunks.
    """
    col_types = {}
    chunk_counter = 0

    for chunk in pd.read_csv(csv_file, chunksize=chunksize, sep=sep, dtype_backend="pyarrow",
                             dtype={"Commune": "string", "Department": "string", "SchoolNear": "string"},
                             na_values=[],
                             keep_default_na=False):
        for col in chunk.columns:
            dtype = chunk[col].dtype
            dict_type = col_types.get(col)

            if col == "RoadNumber":
                print("####################################################")
                print(f"#    Col: {col} | dtype pandas: {dtype} | current schema: {dict_type}")
                print("####################################################")

            if dict_type is None:
                # Première fois qu’on voit la colonne
                if pd.api.types.is_integer_dtype(dtype):
                    # Si la colonne contient des nombres avec padding → string
                    sample = chunk[col].astype(str)
                    # print(f"sample :{sample}")
                    if sample.str.match(r"0\d+").any():
                        # print(f"padding detected for column: {col}")
                        col_types[col] = pa.string()
                    else:
                        col_types[col] = pa.int64()

                elif pd.api.types.is_float_dtype(dtype):
                    col_types[col] = pa.float64()
                elif pd.api.types.is_bool_dtype(dtype):
                    col_types[col] = pa.bool_()
                elif pd.api.types.is_datetime64_any_dtype(dtype):
                    col_types[col] = pa.timestamp("ns")
                elif pd.api.types.is_object_dtype(dtype):
                    # Test spécial pour object
                    col_types[col] = normalize_types(pa.string(), dtype, chunk[col])
                else:
                    col_types[col] = pa.string()
            else:
                # Normalisation avec règles + test object
                col_types[col] = normalize_types(dict_type, dtype, chunk[col])

            if col == "RoadNumber":
                print("####################################################")
                print(f"#    AFTER update schema: {col_types[col]}")
                print("####################################################")

        if verbose:
            print(f"Got {len(chunk.columns)} columns in chunk {chunk_counter}")
            print(chunk.head())
        chunk_counter += 1

    fields = [pa.field(col, dtype) for col, dtype in col_types.items()]
    if verbose:
        print("Final schema fields:")
        print(fields)

    return pa.schema(fields)


def csv_to_parquet(csv_file, parquet_file, chunksize=10000, verbose=False, sep='\t'):
    """
    Convertit un fichier CSV en format Parquet en utilisant pyarrow, pandas.

    Args:
        csv_file(str): Le chemin du fichier CSV.
        parquet_file(str): Le chemin où le fichier Parquet sera sauvegardé.
        chunksize(int, default=10000): Le nombre de lignes à lire par bloc (par défaut 10 000 lignes).
        sep(str): separator of the csv file
        verbose(bool=true): info during execution or not
    """
    
    start = time.time()

    schema = find_schema(csv_file=csv_file, verbose=verbose)

    if verbose:
        print("     - schema found:")
        print(schema)

    writer = None

    for chunk in pd.read_csv(csv_file, chunksize=chunksize, sep=sep, dtype_backend="pyarrow",
                             dtype={"Commune": "string", "Department": "string", "SchoolNear": "string"},
                             na_values=[],
                             keep_default_na=False):
        table = pa.Table.from_pandas(chunk, preserve_index=False, schema=schema)
        if writer is None:
            writer = pq.ParquetWriter(parquet_file, schema)
        writer.write_table(table)
    if writer:
        writer.close()
        end = time.time()
        if verbose:
            print(f"Conversion complete. Parquet file saved to {parquet_file} in {end - start:.2f} secs")

if __name__ == "__main__":
    ############################################################
    # Args:
    ############################################################

    # parquet_file = "data/normal_test.parquet"
    # csv_file = "data/normal.csv"

    csv_file = os.path.join(kh.get_samples_dir(), "AccidentsMedium", "Places.txt")
    parquet_file = os.path.join(kh.get_samples_dir(), "AccidentsMedium", "Places_dbg.parquet")
    verbose = False

    ############################################################

    csv_to_parquet(csv_file, parquet_file, chunksize=10000, verbose=verbose)