#pragma once

#include <sstream>
#include <arrow/io/interfaces.h>
#include <google/cloud/storage/client.h>
#include <google/cloud/status_or.h>
#include <arrow/buffer.h>
#include <arrow/io/interfaces.h>
#include <arrow/status.h>

using namespace google::cloud;

class GCSOutputStream : public arrow::io::OutputStream {
public:
    GCSOutputStream(std::shared_ptr<google::cloud::storage::Client> client,
        const std::string& bucket,
        const std::string& object_name);
   
    ~GCSOutputStream() override = default;

    arrow::Status Write(const void* data, int64_t nbytes) override;
    arrow::Result<int64_t> Tell() const override;
    arrow::Status Close() override;
    bool closed() const override;

private:
    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
    std::string object_name_;
    std::unique_ptr<google::cloud::storage::ObjectWriteStream> writer_;
    bool closed_;
    int64_t position_;
};


class GCSInputStream : public arrow::io::RandomAccessFile {
public:
    GCSInputStream(std::shared_ptr<google::cloud::storage::Client> client,
        const std::string& bucket,
        const std::string& object_name);

    ~GCSInputStream() override = default;

    arrow::Result<int64_t> Read(int64_t nbytes, void* out);

    arrow::Result<std::shared_ptr<arrow::Buffer>> Read(int64_t nbytes);
    arrow::Result<int64_t> GetSize();
    arrow::Result<int64_t> Tell() const override;
    arrow::Status Seek(int64_t position) override;
    arrow::Status Close() override;
    bool closed() const override;

    private:
    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
    std::string object_name_;
    int64_t position_;
	bool closed_;
};