#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/reader.h>

using namespace std::chrono;

int parquet_to_csv(const std::string & input_file, const std::string & output_file) {
    try {
		auto start = high_resolution_clock::now();

		// open the parquet file
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(
            infile, arrow::io::ReadableFile::Open(input_file));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(
            reader,
            parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

        reader->set_use_threads(true);

		// open output CSV file
        std::shared_ptr<arrow::io::OutputStream> outfile;
        auto outfile_res = arrow::io::FileOutputStream::Open(output_file);
        if (!outfile_res.ok()) {
            std::cerr << "Erreur ouverture fichier CSV: " << outfile_res.status().ToString() << std::endl;
            return 1;
        }
        outfile = *outfile_res;


        auto write_options = arrow::csv::WriteOptions::Defaults();
		write_options.include_header = true;

        bool header_written = false;

		// process each row group
        for (int rg = 0; rg < reader->num_row_groups(); rg++) {
            std::shared_ptr<arrow::Table> table;
            PARQUET_THROW_NOT_OK(reader->RowGroup(rg)->ReadTable(&table));

			// write header only for the first row group
            if (!header_written) {
                write_options.include_header = true;
                header_written = true;
            }
            else {
                write_options.include_header = false;
            }
            PARQUET_THROW_NOT_OK(
                arrow::csv::WriteCSV(*table, write_options, outfile.get()));
        }

        PARQUET_THROW_NOT_OK(outfile->Close());

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        std::cout << "Conversion done in " << duration << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error ParquetToCsv : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// ------------------------ Main Conversion ------------------------
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>\n";
        return 1;
    }

    const std::string parquet_file = argv[1];
    const std::string output_file = argv[2];

    return parquet_to_csv(parquet_file, output_file);
}