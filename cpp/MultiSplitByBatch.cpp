#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <regex>

namespace fs = std::filesystem;

// ------------------------ Queue bornée ------------------------
class BatchQueue {
public:
    BatchQueue(size_t max_size) : max_size_(max_size) {}

    void push(std::shared_ptr<arrow::RecordBatch> batch) {
        std::unique_lock<std::mutex> lock(m_);
        cv_producer_.wait(lock, [&] { return q_.size() < max_size_ || finished_; });
        if (finished_) return;
        q_.push(std::move(batch));
        cv_consumer_.notify_one();
    }

    std::shared_ptr<arrow::RecordBatch> pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_consumer_.wait(lock, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return nullptr;
        auto b = q_.front();
        q_.pop();
        cv_producer_.notify_one();
        return b;
    }

    void set_finished() {
        {
            std::lock_guard<std::mutex> lock(m_);
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

// ------------------------ Logger thread-safe ------------------------
std::mutex log_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << msg << std::endl;
}

// ------------------------ Écriture d'un batch en CSV ------------------------
void write_batch_to_csv(std::shared_ptr<arrow::RecordBatch> batch,
    const std::string& filename,
    bool write_header) {
    auto out_res = arrow::io::FileOutputStream::Open(filename);
    if (!out_res.ok()) throw std::runtime_error(out_res.status().ToString());
    auto outfile = *out_res;

    auto buffered_res = arrow::io::BufferedOutputStream::Create(4 * 1024 * 1024,
        arrow::default_memory_pool(),
        outfile);
    if (!buffered_res.ok()) throw std::runtime_error(buffered_res.status().ToString());
    auto buffered_out = *buffered_res;

    arrow::csv::WriteOptions write_options = arrow::csv::WriteOptions::Defaults();
    write_options.include_header = write_header;

    // log("Writing " + filename);
    PARQUET_THROW_NOT_OK(arrow::csv::WriteCSV(*batch, write_options, buffered_out.get()));
    PARQUET_THROW_NOT_OK(buffered_out->Close());
    PARQUET_THROW_NOT_OK(outfile->Close());
    // log("Done " + filename);
}

// Fonction affichage de la liste de fichiers
void dump_files(const std::vector<std::string>& batch_files) {
	log("Batch files:");
    for (const auto& file : batch_files) {
		log("file: " + file);
	}
	log("End of batch files");
}

// Fonction utilitaire pour extraire l’index du batch
int extract_batch_index(const std::string& filename) {
    std::regex re("batch_(\\d+)\\.csv");
    std::smatch match;
    if (std::regex_search(filename, match, re)) {
        return std::stoi(match[1]);
    }
    return -1;
}

// ------------------------ Fusion des CSV en un seul ------------------------
void merge_csv_files(std::vector<std::string> batch_files,
    const std::string& output_file) {

    // Trier les fichiers par index
    std::sort(batch_files.begin(), batch_files.end(),
        [](const std::string& a, const std::string& b) {
            return extract_batch_index(a) < extract_batch_index(b);
        });

	// dump_files(batch_files);
    auto merge_start = std::chrono::high_resolution_clock::now();

    std::ofstream outfile(output_file);
    if (!outfile.is_open()) throw std::runtime_error("Cannot open output file");

    for (size_t i = 0; i < batch_files.size(); i++) {
        std::ifstream infile(batch_files[i]);
        if (!infile.is_open()) throw std::runtime_error("Cannot open batch file");

        std::string line;
        int acc = 0;
        while (std::getline(infile, line)) {
            outfile << line << "\n";
            acc += 1;
        }
        infile.close();
        fs::remove(batch_files[i]);
    }

    outfile.close();

    auto merge_end = std::chrono::high_resolution_clock::now();
    auto merge_ms = std::chrono::duration_cast<std::chrono::milliseconds>(merge_end - merge_start).count();
    std::cout << "Merge done in " << merge_ms << " ms" << std::endl;
}

// ------------------------ Main Conversion ------------------------
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>\n";
        return 1;
    }

    const std::string parquet_file = argv[1];
    const std::string output_file = argv[2];

    auto total_start = std::chrono::high_resolution_clock::now();

    try {
        // Ouvrir le Parquet
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(parquet_file));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));
        reader->set_use_threads(true);

        std::shared_ptr<arrow::RecordBatchReader> batch_reader;
        PARQUET_ASSIGN_OR_THROW(batch_reader, reader->GetRecordBatchReader());

        // --- Queue bornée et pool de writers
        BatchQueue queue(50);
        std::atomic<int> batch_id_counter{ 0 };
        const int num_writer_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> writer_threads;
        std::vector<std::string> batch_files;
        std::mutex batch_files_mutex;

        for (int i = 0; i < num_writer_threads; i++) {
            writer_threads.emplace_back([&]() {
                while (true) {
                    auto batch = queue.pop();
                    if (!batch) break;

                    int batch_id = batch_id_counter.fetch_add(1);
                    std::string filename = "batch_" + std::to_string(batch_id) + ".csv";
                    bool write_header = (batch_id == 0);

                    write_batch_to_csv(batch, filename, write_header);

                    {
                        std::lock_guard<std::mutex> lock(batch_files_mutex);
                        batch_files.push_back(filename);
                    }
                }
                });
        }

        // Producteur
        while (true) {
            std::shared_ptr<arrow::RecordBatch> batch;
            PARQUET_ASSIGN_OR_THROW(batch, batch_reader->Next());
            if (!batch) break;
            queue.push(batch);
        }

        queue.set_finished();

        for (auto& t : writer_threads) t.join();

        // --- Fusion finale avec chrono
         merge_csv_files(batch_files, output_file);

        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();
        std::cout << "Total execution time: " << total_ms << " ms" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
