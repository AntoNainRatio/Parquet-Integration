#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/reader.h>

using namespace std::chrono;

int main(int argc, char** argv) {

    auto start = high_resolution_clock::now();

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>" << std::endl;
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];

    try {
        // --- Ouvrir le fichier Parquet
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(
            infile, arrow::io::ReadableFile::Open(input_file));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(
            reader,
            parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

        reader->set_use_threads(true);

        // --- Préparer le fichier CSV de sortie
        std::shared_ptr<arrow::io::OutputStream> outfile;
        auto outfile_res = arrow::io::FileOutputStream::Open(output_file);
        if (!outfile_res.ok()) {
            std::cerr << "Erreur ouverture fichier CSV: " << outfile_res.status().ToString() << std::endl;
            return 1;
        }
        outfile = *outfile_res;


        auto write_options = arrow::csv::WriteOptions::Defaults();
        write_options.include_header = true; // écrire l'entête une seule fois

        bool header_written = false;

        // --- Parcourir les row groups un par un
        for (int rg = 0; rg < reader->num_row_groups(); rg++) {
            std::shared_ptr<arrow::Table> table;
            PARQUET_THROW_NOT_OK(reader->RowGroup(rg)->ReadTable(&table));

            //On écrit l'entête uniquement sur le premier batch
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
