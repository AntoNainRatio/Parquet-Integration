#pragma once
#include <arrow/io/interfaces.h>
#include <google/cloud/storage/client.h>
#include <sstream>

class GCSOutputStream : public arrow::io::OutputStream {
public:
    GCSOutputStream(std::shared_ptr<google::cloud::storage::Client> client,
        const std::string& bucket,
        const std::string& object_name)
        : client_(std::move(client)),
        bucket_(bucket),
        object_name_(object_name),
        closed_(false) {
    }

    // Écriture dans un buffer mémoire
    arrow::Status Write(const void* data, int64_t nbytes) override {
        if (closed_) {
            return arrow::Status::IOError("Cannot write to closed stream");
        }
        buffer_.write(reinterpret_cast<const char*>(data), nbytes);
        return arrow::Status::OK();
    }

    // Position actuelle (nombre d’octets écrits)
    arrow::Result<int64_t> Tell() const override {
        return static_cast<int64_t>(buffer_.tellp());
    }

    // Fermeture du flux : upload vers GCS
    arrow::Status Close() override {
        if (closed_) return arrow::Status::OK();
        closed_ = true;

        auto writer = client_->WriteObject(bucket_, object_name_);
        std::string data = buffer_.str();
        writer.write(data.data(), data.size());
        writer.Close();

        auto metadata = writer.metadata();
        if (!metadata) {
            return arrow::Status::IOError("Erreur upload GCS: " + metadata.status().message());
        }

        return arrow::Status::OK();
    }

    bool closed() const override { return closed_; }

private:
    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
    std::string object_name_;
    mutable std::ostringstream buffer_;
    bool closed_;
};