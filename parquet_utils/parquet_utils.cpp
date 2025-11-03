#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/reader.h>

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <regex>

#include <arrow/array.h>
#include <arrow/scalar.h>

#include "parquet_utils.h"

namespace fs = std::filesystem;
using namespace std::chrono;

char delimiter = '\t';

struct RowGroupJob {
    int start;
    int end;
    int job_id;
};

class JobQueue {
public:
    void push(const RowGroupJob& job) {
        std::unique_lock<std::mutex> lock(m_);
        q_.push(job);
        cv_.notify_one();
    }

    bool pop(RowGroupJob& job) {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return false;
        job = q_.front();
        q_.pop();
        return true;
    }

    void set_finished() {
        std::unique_lock<std::mutex> lock(m_);
        finished_ = true;
        cv_.notify_all();
    }

private:
    std::queue<RowGroupJob> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool finished_ = false;
};

// thread safe logger
std::mutex log_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << msg << std::endl;
}

void dump_filnames(const std::vector<std::string>& filenames) {
    std::cout << "Filenames:" << std::endl;
    for (const auto& file : filenames) {
        std::cout << '\t' << file << std::endl;
    }
    std::cout << "EOFilenames" << std::endl;
}

// worker processing jobs from the queue
void worker(std::shared_ptr<arrow::io::ReadableFile> infile, JobQueue& queue, std::mutex& chunk_mutex, std::string& prefix, std::string& output_dir) {
    while (true) {
        RowGroupJob job;
        if (!queue.pop(job)) break;

        try {
            std::unique_ptr<parquet::arrow::FileReader> reader;
            PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

            std::ostringstream filename;
            filename << output_dir << "/" << prefix << job.start << ".txt";
            std::shared_ptr<arrow::io::FileOutputStream> outfile;
            PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(filename.str()));

            arrow::csv::WriteOptions write_options = arrow::csv::WriteOptions::Defaults();
            write_options.include_header = (job.start == 0);
            write_options.delimiter = delimiter;
            write_options.null_string = "";
            write_options.quoting_style = arrow::csv::QuotingStyle::Needed;

            for (int rg = job.start; rg < job.end; rg++) {
                std::shared_ptr<arrow::Table> table;
                PARQUET_THROW_NOT_OK(reader->ReadRowGroup(rg, &table));

                PARQUET_THROW_NOT_OK(arrow::csv::WriteCSV(*table, write_options, outfile.get()));
                write_options.include_header = false;
            }

            PARQUET_THROW_NOT_OK(outfile->Close());

            //log("Processed job " + std::to_string(job.job_id));

            std::lock_guard<std::mutex> lock(chunk_mutex);
        }
        catch (const std::exception& e) {
            log("Error in job " + std::to_string(job.job_id) + ": " + e.what());
        }
    }
}

bool isQuotingNeeded(const std::string& value, const char separator) {
    for(const char& c: value) {
        if (c == '"' || c == separator || c == '\n' || c == '\r') {
            return true;
        }
	}
    return false;
}

void worker_gpt(std::shared_ptr<arrow::io::ReadableFile> infile,
    JobQueue& queue,
    std::mutex& chunk_mutex,
    std::string& prefix,
    std::string& output_dir) {

    while (true) {
        RowGroupJob job;
        if (!queue.pop(job)) break;

        try {
            std::unique_ptr<parquet::arrow::FileReader> reader;
            PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

            std::ostringstream filename;
            filename << output_dir << "/" << prefix << job.start << ".txt";
            std::ofstream outfile(filename.str(), std::ios::out | std::ios::trunc);

            if (!outfile.is_open()) {
                log("Error: cannot open output file " + filename.str());
                continue;
            }

            for (int rg = job.start; rg < job.end; rg++) {
                std::shared_ptr<arrow::Table> table;
                PARQUET_THROW_NOT_OK(reader->ReadRowGroup(rg, &table));

                int64_t num_columns = table->num_columns();
                int64_t num_rows = table->num_rows();

                // Write header only once (first job + first group)
                if (job.start == 0 && rg == job.start) {
                    for (int64_t c = 0; c < num_columns; c++) {
                        outfile << table->schema()->field(static_cast<int>(c))->name();
                        if (c < num_columns - 1) outfile << '\t';
                    }
                    outfile << '\n';
                }

                // Loop rows
                for (int64_t r = 0; r < num_rows; r++) {
                    for (int64_t c = 0; c < num_columns; c++) {
                        std::shared_ptr<arrow::ChunkedArray> chunked = table->column(static_cast<int>(c));
                        int64_t row_in_chunks = r;
                        std::shared_ptr<arrow::Scalar> scalar;

                        // Find correct chunk
                        for (const auto& chunk : chunked->chunks()) {
                            int64_t len = chunk->length();
                            if (row_in_chunks < len) {
                                PARQUET_ASSIGN_OR_THROW(std::shared_ptr<arrow::Scalar> s, chunk->GetScalar(row_in_chunks));
                                scalar = s;
                                break;
                            }
                            row_in_chunks -= len;
                        }

                        std::string value;
                        if (!scalar || !scalar->is_valid) {
                            value = "";
                        }
                        else {
                            switch (scalar->type->id()) {
                            case arrow::Type::STRING:
                            case arrow::Type::LARGE_STRING: {
                                auto s = std::static_pointer_cast<arrow::StringScalar>(scalar);
                                value = s->value ? s->value->ToString() : "";

                                // --- Apply CSV quoting rules only for strings ---
                                bool must_quote = isQuotingNeeded(value, delimiter);

                                if (must_quote) {
                                    // Double internal quotes
                                    std::string escaped;
                                    escaped.reserve(value.size());
                                    for (char ch : value) {
                                        if (ch == '"') escaped += "\"\"";
                                        else escaped += ch;
                                    }
                                    value = "\"" + escaped + "\"";
                                }
                                break;
                            }

                            default:
                                value = scalar->ToString();
                                break;
                            }
                        }

                        outfile << value;
                        if (c < num_columns - 1) outfile << '\t';
                    }
                    outfile << '\n';
                }
            }

            outfile.close();

            std::lock_guard<std::mutex> lock(chunk_mutex);
        }
        catch (const std::exception& e) {
            log("Error in job " + std::to_string(job.job_id) + ": " + e.what());
        }
    }
}



// get chunk index based on the name of the csv
int extract_chunk_index(const std::string& filename) {
    std::regex re("(\\d+)\\.txt$");
    std::smatch match;
    if (std::regex_search(filename, match, re)) return std::stoi(match[1]);
    return -1;
}

// Conversion Parquet -> CSV with multithreading
// each thread processes several row groups
// row_groups per thread is set by total_row_groups / num_threads
// this way work is divided by number of threads available
int parquetToCsv(const char* parquet_file, const char* prefix, const char* output_dir) {

    auto total_start = high_resolution_clock::now();

    try {
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(parquet_file));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

        int total_row_groups = reader->parquet_reader()->metadata()->num_row_groups();
        // log("Total row groups: " + std::to_string(total_row_groups));

        int num_threads = std::min(total_row_groups, (int)std::thread::hardware_concurrency()); // do not get more threads than row groups

        int row_groups_per_thread = total_row_groups / num_threads;
        // log("Using " + std::to_string(num_threads) + " threads, each processing approximately " + std::to_string(row_groups_per_thread) + " row groups");

        int extra = total_row_groups % num_threads;

        // managing jobs and row_groups
        JobQueue queue;
        int job_id = 0;

        // creating dir if doesn't exist
        fs::create_directories(output_dir);

        int start = 0;
        for (int i = 0; i < num_threads; i++) {
            int count = row_groups_per_thread + (i < extra ? 1 : 0); // add one more if in the extra count
            int end = std::min(start + count, total_row_groups);

            queue.push(RowGroupJob{ start, end, job_id++ });
            start = end;
        }



        std::vector<std::thread> threads;
        std::mutex chunk_mutex;

        std::string prefix_string = prefix;
        std::string output_dir_string = output_dir;

        // starting worker threads
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(worker, infile, std::ref(queue), std::ref(chunk_mutex), std::ref(prefix_string), std::ref(output_dir_string));
        }

        queue.set_finished();

        // waiting for all threads to finish
        for (auto& t : threads) t.join();


        auto total_end = high_resolution_clock::now();
        std::cout << "Total execution time: " << duration_cast<milliseconds>(total_end - total_start) << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

void merge_csv_files(const char* dir, const char* prefix, const char* output_file) {

	std::vector<std::string> chunk_files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            const std::string& filename = entry.path().filename().string();
            if (filename.rfind(prefix, 0) == 0 && filename.size() > strlen(prefix) && filename.substr(filename.size() - 4) == ".txt") {
                chunk_files.push_back(entry.path().string());
            }
        }
	}

    // sorting chunk filename list to merge them in right order
    std::sort(chunk_files.begin(), chunk_files.end(),
        [](const std::string& a, const std::string& b) { return extract_chunk_index(a) < extract_chunk_index(b); });


    auto merge_start = high_resolution_clock::now();
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) throw std::runtime_error("Cannot open output file");

    for (const auto& file : chunk_files) {
        std::ifstream infile(file);
        if (!infile.is_open()) throw std::runtime_error("Cannot open chunk file " + file);

        std::string line;
        while (std::getline(infile, line)) {
            outfile << line << "\n";
        }
        infile.close();
    }
    outfile.close();
    auto merge_end = high_resolution_clock::now();
    std::cout << "Merge done in " << duration_cast<milliseconds>(merge_end - merge_start) << std::endl;
}

bool delete_dir(const char* output_dir, const char* prefix) {
    std::filesystem::path search_path = std::filesystem::path(output_dir);
	std::string pattern = std::string(prefix);
    if (!std::filesystem::exists(search_path) || !std::filesystem::is_directory(search_path)) {
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(search_path)) {
        if (entry.is_regular_file()) {
            const std::string& filename = entry.path().filename().string();
			// Check if the filename starts with the given prefix and ends with .txt
            if (filename.rfind(pattern, 0) == 0 && filename.size() > pattern.size() && filename.substr(filename.size() - 4) == ".txt") {
                continue;
			}
            else
            {
                return false;
            }
        }
    }
	return fs::remove_all(search_path);
}


