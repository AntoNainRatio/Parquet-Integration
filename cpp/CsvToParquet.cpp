#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/writer.h>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.csv> <output.parquet>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    try {
        // --- Ouvrir le CSV en entrée
        ARROW_ASSIGN_OR_RAISE(auto infile,
            arrow::io::ReadableFile::Open(input_file));

        // Options de lecture CSV
        auto read_options = arrow::csv::ReadOptions::Defaults();
        read_options.block_size = 1 << 20; // ~1 MB par chunk lu
        auto parse_options = arrow::csv::ParseOptions::Defaults();
        auto convert_options = arrow::csv::ConvertOptions::Defaults();

        // Créer le lecteur de batch CSV
        ARROW_ASSIGN_OR_RAISE(
            auto csv_reader,
            arrow::csv::StreamingReader::Make(
                arrow::io::default_io_context(),
                infile,
                read_options,
                parse_options,
                convert_options
            )
        );

        // --- Préparer la sortie Parquet
        ARROW_ASSIGN_OR_RAISE(auto outfile,
            arrow::io::FileOutputStream::Open(output_file));

        std::shared_ptr<parquet::arrow::FileWriter> parquet_writer;

        std::shared_ptr<arrow::RecordBatch> batch;
        bool first_batch = true;

        // Lire batch par batch
        while (true) {
            auto st = csv_reader->ReadNext(&batch);
            if (!st.ok()) {
                std::cerr << "Erreur lecture batch CSV: " << st.ToString() << std::endl;
                return 1;
            }
            if (!batch) break; // plus de données

            if (first_batch) {
                // Init writer Parquet avec le schéma du 1er batch
                PARQUET_THROW_NOT_OK(parquet::arrow::FileWriter::Open(
                    *batch->schema(),
                    arrow::default_memory_pool(),
                    outfile,
                    parquet::WriterProperties::Builder().build(),
                    &parquet_writer
                ));
                first_batch = false;
            }

            // Écrire le batch courant dans le Parquet
            PARQUET_THROW_NOT_OK(parquet_writer->WriteRecordBatch(*batch));
        }

        // Fermer le writer Parquet
        if (parquet_writer) {
            PARQUET_THROW_NOT_OK(parquet_writer->Close());
        }

        PARQUET_THROW_NOT_OK(outfile->Close());

        std::cout << "Conversion CSV -> Parquet terminée : " << output_file << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
