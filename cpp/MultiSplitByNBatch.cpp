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
using namespace std::chrono;

struct BatchChunk {
    int chunk_id;
    std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
};;

class ChunkQueue {
public:
    ChunkQueue(size_t max_size) : max_size_(max_size) {}

    void push(BatchChunk chunk) {
        std::unique_lock<std::mutex> lock(m_);
        cv_producer_.wait(lock, [&] { return q_.size() < max_size_ || finished_; });
        if (finished_) return;
        q_.push(std::move(chunk));
        cv_consumer_.notify_one();
    }

    BatchChunk pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_consumer_.wait(lock, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return BatchChunk{ -1, {} };
        auto chunk = q_.front();
        q_.pop();
        cv_producer_.notify_one();
        return chunk;
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
    std::queue<BatchChunk> q_;
    std::mutex m_;
    std::condition_variable cv_producer_;
    std::condition_variable cv_consumer_;
    size_t max_size_;
    bool finished_ = false;
};

// thread safe logger
std::mutex log_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << msg << std::endl;
}

// write a batch chunk into a csv file
void write_chunk_to_csv(const BatchChunk& chunk, const std::string& filename, bool write_header) {
    auto out_res = arrow::io::FileOutputStream::Open(filename);
    if (!out_res.ok()) throw std::runtime_error(out_res.status().ToString());
    auto outfile = *out_res;

    auto buffered_res = arrow::io::BufferedOutputStream::Create(4 * 1024 * 1024,
        arrow::default_memory_pool(), outfile);
    if (!buffered_res.ok()) throw std::runtime_error(buffered_res.status().ToString());
    auto buffered_out = *buffered_res;

    arrow::csv::WriteOptions write_options = arrow::csv::WriteOptions::Defaults();
    write_options.include_header = write_header;

    for (size_t i = 0; i < chunk.batches.size(); i++) {
        if (i > 0) write_options.include_header = false; // header seulement une fois par chunk
        PARQUET_THROW_NOT_OK(arrow::csv::WriteCSV(*chunk.batches[i], write_options, buffered_out.get()));
    }

    PARQUET_THROW_NOT_OK(buffered_out->Close());
    PARQUET_THROW_NOT_OK(outfile->Close());
}

// dump of the batch_files list
void dump_files(const std::vector<std::string>& batch_files) {
    log("Batch files:");
    for (const auto& file : batch_files) {
        log("file: " + file);
    }
    log("End of batch files");
}

// get chunk index based on the name of the csv
int extract_chunk_index(const std::string& filename) {
    std::regex re("chunk_(\\d+)\\.csv");
    std::smatch match;
    if (std::regex_search(filename, match, re)) return std::stoi(match[1]);
    return -1;
}

// ------------------------ Fusion des CSV en un seul ------------------------
void merge_csv_files(std::vector<std::string>& chunk_files, const std::string& output_file) {
    // sorting chunk filename list to merge them in right order
    std::sort(chunk_files.begin(), chunk_files.end(),
        [](const std::string& a, const std::string& b) { return extract_chunk_index(a) < extract_chunk_index(b); });

    auto merge_start = high_resolution_clock::now();

    std::ofstream outfile(output_file);
    if (!outfile.is_open()) throw std::runtime_error("Cannot open output file");

    for (size_t i = 0; i < chunk_files.size(); i++) {
        std::ifstream infile(chunk_files[i]);
        if (!infile.is_open()) throw std::runtime_error("Cannot open chunk file");

        std::string line;
        while (std::getline(infile, line)) {
            outfile << line << "\n";
        }
        infile.close();
        fs::remove(chunk_files[i]); // deleting temporary file
    }
    outfile.close();
    auto merge_end = high_resolution_clock::now();
    std::cout << "Merge done in " <<
        duration_cast<milliseconds>(merge_end - merge_start).count() << " ms" << std::endl;
}

void delete_chunk_files(const std::vector<std::string>& chunk_files) {
    for (const auto& file : chunk_files) {
        fs::remove(file);
    }
}

size_t get_length(std::shared_ptr<parquet::arrow::FileReader> reader) {
    std::shared_ptr<arrow::RecordBatchReader> batch_reader;
    PARQUET_ASSIGN_OR_THROW(batch_reader, reader->GetRecordBatchReader());

    size_t res = 0;

    while (true) {
        std::shared_ptr<arrow::RecordBatch> batch;
        PARQUET_ASSIGN_OR_THROW(batch, batch_reader->Next());
        if (!batch) break;
        res++;
    }
    return res;
}

std::vector<std::string> parquet_to_csv(const std::string& parquet_file, const std::string& output_file, const size_t batches_per_chunk = 2, const bool merging = true) {
    auto total_start = high_resolution_clock::now();
    try {
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(parquet_file));

        std::shared_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));
        reader->set_use_threads(true);

        auto start_length = high_resolution_clock::now();
        const size_t length = get_length(reader);
        auto end_length = high_resolution_clock::now();

        auto total_end = high_resolution_clock::now();
        std::cout << "get_length = " << length <<" | time: " <<
            duration_cast<milliseconds>(end_length - start_length) << std::endl;


        std::shared_ptr<arrow::RecordBatchReader> batch_reader;
        PARQUET_ASSIGN_OR_THROW(batch_reader, reader->GetRecordBatchReader());


        ChunkQueue queue(50);
        std::atomic<int> chunk_id_counter{ 0 };
        const int num_writer_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> writer_threads;
        std::vector<std::string> chunk_files;
        std::mutex chunk_files_mutex;

        for (int i = 0; i < num_writer_threads; i++) {
            writer_threads.emplace_back([&]() {
                while (true) {
                    BatchChunk chunk = queue.pop();
                    if (chunk.chunk_id == -1) break;

                    std::string filename = "chunk_" + std::to_string(chunk.chunk_id) + ".csv";
                    bool write_header = (chunk.chunk_id == 0);

                    write_chunk_to_csv(chunk, filename, write_header);

                    {
                        std::lock_guard<std::mutex> lock(chunk_files_mutex);
                        chunk_files.push_back(filename);
                    }
                }
                });
        }

        /////////////////////////////////////////////////////////////////////////////////////

        // Producer
        std::vector<std::shared_ptr<arrow::RecordBatch>> current_batches;
        while (true) {
            std::shared_ptr<arrow::RecordBatch> batch;
            PARQUET_ASSIGN_OR_THROW(batch, batch_reader->Next());
            if (!batch) break;

            current_batches.push_back(batch);
            if (current_batches.size() >= batches_per_chunk) {
                int chunk_id = chunk_id_counter.fetch_add(1);
                queue.push(BatchChunk{ chunk_id, std::move(current_batches) });
                current_batches.clear();
            }
        }

        // Manage last chunk if any
        if (!current_batches.empty()) {
            int chunk_id = chunk_id_counter.fetch_add(1);
            queue.push(BatchChunk{ chunk_id, std::move(current_batches) });
        }

        //////////////////////////////////////////////////////////////////////////////////////

        queue.set_finished();

        // waiting for writers to finish
        for (auto& t : writer_threads) t.join();

        auto split_end = high_resolution_clock::now();
        std::cout << "Splitting done in " <<
            duration_cast<milliseconds>(split_end - total_start) << std::endl;

        // merging
        if (merging) {
            merge_csv_files(chunk_files, output_file);

            auto total_end = high_resolution_clock::now();
            std::cout << "Total execution time: " <<
                duration_cast<milliseconds>(total_end - total_start) << std::endl;
        }
        return chunk_files;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return {};
    }
}



// ------------------------ Main Conversion ------------------------
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>\n";
        return 1;
    }

    const std::string parquet_file = argv[1];
    const std::string output_file = argv[2];

	const std::vector<int> batch_sizes = {3};
    const std::vector<std::string> inputs = { 
        "C:/Users/Public/khiops_data/samples/Accidents/Users.parquet",
        "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet",
        "C:/Users/Public/khiops_data/samples/AccidentsHeavy/Users_heavy.parquet",
        "C:/Users/Public/khiops_data/samples/AccidentsVeryHeavy/Users_veryheavy.parquet",
    };

    const std::vector<std::string> outputs = { 
        "C:/Users/Public/khiops_data/samples/Accidents/Users_cpp.txt",
        "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium_cpp.txt",
        "C:/Users/Public/khiops_data/samples/AccidentsHeavy/Users_heavy_cpp.txt",
        "C:/Users/Public/khiops_data/samples/AccidentsVeryHeavy/Users_veryheavy_cpp.txt",
    };

    const bool merging = false;

 //   for (const int n: batch_sizes) {
 //       std::cout << "Processing with batch size: " << n << std::endl;
 //       const std::vector<std::string> chunk_files = parquet_to_csv(parquet_file, output_file, n, merging);
	//	std::cout << "----------------------------------------" << std::endl;

 //       // still deletes temp files
 //       if (!merging) {
 //           delete_chunk_files(chunk_files);
	//	}
	//}

    for (int i = 0; i < inputs.size(); i++) {
        std::cout << "Processing " << inputs[i] << " ---> " << outputs[i] << std::endl;
        const std::vector<std::string> chunk_files = parquet_to_csv(inputs[i], outputs[i], 2, merging);
        std::cout << "----------------------------------------" << std::endl;

        // still deletes temp files
        if (!merging) {
            delete_chunk_files(chunk_files);
        }
    }


    return 0;
}
