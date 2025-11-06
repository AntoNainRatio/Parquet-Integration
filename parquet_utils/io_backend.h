#include <memory>
#include <arrow/io/file.h>
#include <arrow/io/type_fwd.h>
#include <arrow/result.h>

#include "google/cloud/rest_options.h"
#include "google/cloud/storage/client.h"
#include <google/cloud/storage/object_write_stream.h>

class StorageBackend {
public:
    virtual std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) = 0;
    virtual std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) = 0;
    virtual ~StorageBackend() = default;
};

class LocalBackend : public StorageBackend {
public:
    std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) override {
        // OpenFile renvoie un arrow::Result<std::shared_ptr<ReadableFile>>
        arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> result =
            arrow::io::ReadableFile::Open(path);

        if (!result.ok()) {
            throw std::runtime_error("Erreur lors de l'ouverture du fichier en lecture : " +
                result.status().ToString());
        }
        return result.ValueOrDie();
    }

    std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) override {
        arrow::Result<std::shared_ptr<arrow::io::FileOutputStream>> result =
            arrow::io::FileOutputStream::Open(path);

        if (!result.ok()) {
            throw std::runtime_error("Erreur lors de l'ouverture du fichier en écriture : " +
                result.status().ToString());
        }
        return result.ValueOrDie();
    }
};

class GCSBackend : public StorageBackend {
public:
    GCSBackend(std::shared_ptr<google::cloud::storage::Client> client, const std::string& bucket)
        : client_(client), bucket_(bucket) {
    }

    std::shared_ptr<arrow::io::RandomAccessFile> openInput(const std::string& path) override {
        // Ici tu pourrais implémenter une classe GCSRandomAccessFile
        // ou lire en mémoire via client_->ReadObject(bucket_, path)
        throw std::runtime_error("Lecture GCS non encore implémentée");
    }

    std::shared_ptr<arrow::io::OutputStream> openOutput(const std::string& path) override {
        // Pareil : soit un wrapper OutputStream, soit écriture buffer + upload
        throw std::runtime_error("Écriture GCS non encore implémentée");
    }

private:
    std::shared_ptr<google::cloud::storage::Client> client_;
    std::string bucket_;
};

std::unique_ptr<StorageBackend> select_backend(const std::string& uri);
