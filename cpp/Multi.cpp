#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/reader.h>

using namespace std::chrono;

// Thread-safe queue bornée
class BatchQueue {
public:
    BatchQueue(size_t max_size) : max_size_(max_size) {}

    void push(std::shared_ptr<arrow::RecordBatch> batch) {
        std::unique_lock<std::mutex> lock(m_);
        cv_producer_.wait(lock, [&] { return q_.size() < max_size_ || finished_; });
        if (finished_) return; // si terminé, on ne pousse plus
        q_.push(batch);
        cv_consumer_.notify_one();
    }

    std::shared_ptr<arrow::RecordBatch> pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_consumer_.wait(lock, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return nullptr;
        auto batch = q_.front();
        q_.pop();
        cv_producer_.notify_one();
        return batch;
    }

    void set_finished() {
        {
            std::unique_lock<std::mutex> lock(m_);
            finished_ = true;
        }
        cv_producer_.notify_all();
        cv_consumer_.notify_all();
    }

private:
    std::queue<std::shared_ptr<arrow::RecordBatch>> q_;
    std::mutex m_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    size_t max_size_;
    bool finished_ = false;
};

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>" << std::endl;
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];

    try {
        auto start = high_resolution_clock::now();

        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(input_file));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));
        reader->set_use_threads(true);

        // Préparer fichier CSV avec buffer
        auto raw_out_res = arrow::io::FileOutputStream::Open(output_file);
        if (!raw_out_res.ok()) throw std::runtime_error(raw_out_res.status().ToString());
        auto outfile = *raw_out_res;

        auto buffered_out_res = arrow::io::BufferedOutputStream::Create(4 * 1024 * 1024,
            arrow::default_memory_pool(),
            outfile);
        if (!buffered_out_res.ok()) throw std::runtime_error(buffered_out_res.status().ToString());
        auto buffered_out = *buffered_out_res;

        arrow::csv::WriteOptions write_options = arrow::csv::WriteOptions::Defaults();
        write_options.include_header = true;

        BatchQueue queue(100);  // Limite à 100 batches en mémoire
        bool header_written = false;

        // Thread writer
        std::thread writer_thread([&]() {
            while (true) {
                auto batch = queue.pop();
                if (!batch) break;

                write_options.include_header = !header_written;
                PARQUET_THROW_NOT_OK(arrow::csv::WriteCSV(*batch, write_options, buffered_out.get()));
                header_written = true;
            }
            });

        std::shared_ptr<arrow::RecordBatchReader> batch_reader;
        PARQUET_ASSIGN_OR_THROW(batch_reader, reader->GetRecordBatchReader());

        std::shared_ptr<arrow::RecordBatch> batch;
        while (true) {
            PARQUET_ASSIGN_OR_THROW(batch, batch_reader->Next());
            if (!batch) break;
            queue.push(batch);
        }



        queue.set_finished();
        writer_thread.join();

        PARQUET_THROW_NOT_OK(buffered_out->Close());
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
