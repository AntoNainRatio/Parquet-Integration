#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <parquet/arrow/reader.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

#include "parquet_to_csv.h"

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
void worker(std::shared_ptr<arrow::io::ReadableFile> infile, JobQueue& queue, std::vector<std::string>& chunk_files, std::mutex& chunk_mutex, boost::uuids::uuid uuid) {
    while (true) {
        RowGroupJob job;
        if (!queue.pop(job)) break;

        try {
            std::unique_ptr<parquet::arrow::FileReader> reader;
            PARQUET_ASSIGN_OR_THROW(reader, parquet::arrow::OpenFile(infile, arrow::default_memory_pool()));

            std::ostringstream filename;
            filename << uuid << "_" << job.start << ".csv";
            std::shared_ptr<arrow::io::FileOutputStream> outfile;
            PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(filename.str()));

            arrow::csv::WriteOptions write_options = arrow::csv::WriteOptions::Defaults();
            write_options.include_header = (job.start == 0);
            for (int rg = job.start; rg < job.end; rg++) {
                std::shared_ptr<arrow::Table> table;
                PARQUET_THROW_NOT_OK(reader->ReadRowGroup(rg, &table));

                PARQUET_THROW_NOT_OK(arrow::csv::WriteCSV(*table, write_options, outfile.get()));
                write_options.include_header = false;
            }

            PARQUET_THROW_NOT_OK(outfile->Close());

            //log("Processed job " + std::to_string(job.job_id));

            std::lock_guard<std::mutex> lock(chunk_mutex);
            chunk_files.push_back(filename.str());
        }
        catch (const std::exception& e) {
            log("Error in job " + std::to_string(job.job_id) + ": " + e.what());
        }
    }
}

// get chunk index based on the name of the csv
int extract_chunk_index(const std::string& filename) {
    std::regex re("_(\\d+)\\.csv$");
    std::smatch match;
    if (std::regex_search(filename, match, re)) return std::stoi(match[1]);
    return -1;
}

// Conversion Parquet -> CSV with multithreading
// each thread processes several row groups
// row_groups per thread is set by total_row_groups / num_threads
// this way work is divided by number of threads available
std::vector<std::string> parquetToCsv(const char* parquet_file) {

    auto total_start = high_resolution_clock::now();

    std::vector<std::string> chunk_files;

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

        auto uuid = boost::uuids::random_generator()();

		int start = 0;
        for (int i = 0; i < num_threads; i++) {
            int count = row_groups_per_thread + (i < extra ? 1 : 0); // add one more if in the extra count
			int end = std::min(start + count, total_row_groups);

			queue.push(RowGroupJob{ start, end, job_id++ });
			start = end;
		}

        

        std::vector<std::thread> threads;
        std::mutex chunk_mutex;

		// starting worker threads
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(worker, infile, std::ref(queue), std::ref(chunk_files), std::ref(chunk_mutex), std::ref(uuid));
        }

        queue.set_finished();

		// waiting for all threads to finish
        for (auto& t : threads) t.join();


        auto total_end = high_resolution_clock::now();
        //std::cout << "Total execution time: " << duration_cast<milliseconds>(total_end - total_start) << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // sorting chunk filename list
    std::sort(chunk_files.begin(), chunk_files.end(),
        [](const std::string& a, const std::string& b) { return extract_chunk_index(a) < extract_chunk_index(b); });

    return chunk_files;
}

void delete_chunk_files(const std::vector<std::string>& chunk_files) {
    for (const auto& file : chunk_files) {
        fs::remove(file);
    }
}


