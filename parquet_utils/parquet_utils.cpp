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

#include "parquet_utils.h"

namespace fs = std::filesystem;
using namespace std::chrono;

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
            write_options.delimiter = '\t';
            write_options.null_string = "";
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
        //std::cout << "Total execution time: " << duration_cast<milliseconds>(total_end - total_start) << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
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


