#include <memory>
#include <arrow/io/file.h>
#include <arrow/io/type_fwd.h>
#include <arrow/result.h>

#include "google/cloud/rest_options.h"
#include "google/cloud/storage/client.h"
#include <google/cloud/storage/object_write_stream.h>

#include "gcs_stream.h"

class StorageBackend {
public:
    virtual std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) = 0;
    virtual std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) = 0;
    virtual ~StorageBackend() = default;
};

class LocalBackend : public StorageBackend {
public:
    std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) override {
        arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> result =
            arrow::io::ReadableFile::Open(path);

        if (!result.ok()) {
            throw std::runtime_error("Error while opening input of the file : " +
                result.status().ToString());
        }
        return result.ValueOrDie();
    }

    std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) override {
        arrow::Result<std::shared_ptr<arrow::io::FileOutputStream>> result =
            arrow::io::FileOutputStream::Open(path);

        if (!result.ok()) {
            throw std::runtime_error("Error while opening output of the file : " +
                result.status().ToString());
        }
        return result.ValueOrDie();
    }
};

class GCSBackend : public StorageBackend {
public:
    GCSBackend(std::shared_ptr<google::cloud::storage::Client> client,
        const std::string& bucket)
        : client_(std::move(client)), bucket_(bucket) {
    }

    std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) override {
        std::string object_name = extractObjectName(path);
        auto stream = std::make_shared<GCSInputStream>(client_, bucket_, object_name);
        return stream;
    }

    std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) override {
        std::string object_name = extractObjectName(path);
        auto stream = std::make_shared<GCSOutputStream>(client_, bucket_, object_name);
        return stream;
    }

private:
    std::string extractObjectName(const std::string& uri) {
        // Usage: get "path/to/file.txt" from "gs://bucket/path/to/file.txt"
        if (uri.rfind("gs://", 0) == 0) {
            // find the first '/' after "gs://"
            size_t first_slash = uri.find('/', 5);
            if (first_slash == std::string::npos) return "";
            return uri.substr(first_slash + 1);
        }
        return uri;
    }

    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
};

std::unique_ptr<StorageBackend> select_backend(const std::string& uri);
