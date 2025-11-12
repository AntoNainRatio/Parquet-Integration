#pragma once

#include <sstream>
#include <arrow/io/interfaces.h>
#include <google/cloud/storage/client.h>
#include <google/cloud/status_or.h>
#include <arrow/buffer.h>

using namespace google::cloud;

#include <google/cloud/storage/client.h>
#include <arrow/io/interfaces.h>
#include <arrow/status.h>

class GCSOutputStream : public arrow::io::OutputStream {
public:
    GCSOutputStream(std::shared_ptr<google::cloud::storage::Client> client,
        const std::string& bucket,
        const std::string& object_name)
        : client_(std::move(client)),
        bucket_(bucket),
        object_name_(object_name),
        closed_(false),
        position_(0)
    {
        // Ouvre un flux d'écriture GCS immédiatement
        writer_ = std::make_unique<google::cloud::storage::ObjectWriteStream>(
            client_->WriteObject(bucket_, object_name_));
    }

    arrow::Status Write(const void* data, int64_t nbytes) override {
        if (closed_) {
            return arrow::Status::IOError("Cannot write to closed stream");
        }

        if (!writer_ || !*writer_) {
            return arrow::Status::IOError("Invalid GCS writer");
        }

        writer_->write(reinterpret_cast<const char*>(data), nbytes);
        if (!writer_->good()) {
            return arrow::Status::IOError("Erreur d’écriture dans le flux GCS");
        }

        position_ += nbytes;
        return arrow::Status::OK();
    }

    arrow::Result<int64_t> Tell() const override {
        return position_;
    }

    arrow::Status Close() override {
        if (closed_) return arrow::Status::OK();
        closed_ = true;

        if (writer_) {
            writer_->Close();
            auto metadata = writer_->metadata();
            if (!metadata) {
                return arrow::Status::IOError("Erreur upload GCS: " + metadata.status().message());
            }
            writer_.reset();
        }

        return arrow::Status::OK();
    }

    bool closed() const override { return closed_; }

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
        const std::string& object_name)
        : client_(std::move(client)),
        bucket_(bucket),
        object_name_(object_name),
        position_(0),
        closed_(false) {}

    arrow::Result<int64_t> Read(int64_t nbytes, void* out) {
        if (closed_) return arrow::Status::IOError("Stream déjà fermé.");

        StatusOr<google::cloud::storage::ObjectReadStream> stream_or =
            client_->ReadObject(bucket_, object_name_,
                google::cloud::storage::ReadRange(position_, position_ + nbytes));

        if (!stream_or) {
            return arrow::Status::IOError("Erreur lors de la lecture GCS : ",
                stream_or.status().message());
        }

        auto& stream = *stream_or;
        stream.read(reinterpret_cast<char*>(out), nbytes);
        std::streamsize bytes_read = stream.gcount();

        position_ += bytes_read;
        return static_cast<int64_t>(bytes_read);
    }

    arrow::Result<std::shared_ptr<arrow::Buffer>> Read(int64_t nbytes) {
        ARROW_ASSIGN_OR_RAISE(auto buffer, arrow::AllocateResizableBuffer(nbytes));
        ARROW_ASSIGN_OR_RAISE(auto bytes_read, Read(nbytes, buffer->mutable_data()));

		// Update buffer size if read less than requested
        if (bytes_read < nbytes) {
            ARROW_ASSIGN_OR_RAISE(auto slice, buffer->CopySlice(0, bytes_read));
            return slice;
        }

        return buffer;
    }

    arrow::Result<int64_t> GetSize() {
        StatusOr<google::cloud::storage::ObjectMetadata> meta = client_->GetObjectMetadata(bucket_, object_name_);
        if (!meta) {
            return arrow::Status::IOError("Error getting GCS metadata for size: ",
                meta.status().message());
        }
        return static_cast<int64_t>(meta->size());
    }

    arrow::Result<int64_t> Tell() const override {
        return position_;
	}

    arrow::Status Seek(int64_t position) override {
        if (position < 0) {
            return arrow::Status::IOError("Negative seek position");
        }
        position_ = position;
        return arrow::Status::OK();
    }

    arrow::Status Close() override {
        closed_ = true;
        return arrow::Status::OK();
	}

	bool closed() const override { return closed_; }

    private:
    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
    std::string object_name_;
    int64_t position_;
	bool closed_;
};