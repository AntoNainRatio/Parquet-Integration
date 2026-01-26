#include <memory>
#include <arrow/io/type_fwd.h>
#include <arrow/result.h>\

#include "google/cloud/rest_options.h"
#include "google/cloud/storage/client.h"
#include <google/cloud/storage/object_write_stream.h>

#include "io_backend.h"

std::unique_ptr<StorageBackend> select_backend(const std::string& uri) {
    if (uri.rfind("gs://", 0) == 0) {
        std::string bucket, object;
        size_t first_slash = uri.find('/', 5);
        bucket = uri.substr(5, first_slash - 5);

        auto client = std::make_shared<google::cloud::storage::Client>(
            google::cloud::storage::Client::CreateDefaultClient().value()
        );
        return std::make_unique<GCSBackend>(client, bucket);
    }
    else if (uri.rfind("s3://", 0) == 0) {
        throw std::runtime_error("Backend S3 non encore implémenté");
    }
    else {
        // default case
        return std::make_unique<LocalBackend>();
    }
}
