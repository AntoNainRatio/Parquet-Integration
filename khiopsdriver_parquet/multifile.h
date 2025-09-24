#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>     // pour FILE*

//enum class MultiFileMode {
//    READ_ONLY,
//    READ_WRITE
//};

enum class MultiFileError {
    OK = 0,
    NO_FILES,
    OPEN_FAILED,
    IO_ERROR,
    SEEK_ERROR
};

enum class MultiFileWhence {
    BEGIN = 0,
    CURRENT = 1,
    END = 2
};

// multi-file structure acting as a single large file
struct MultiFile {

	std::vector<const char*> filenames;
	std::vector<uint64_t> file_sizes;
    std::vector<uint64_t> prefix_offsets;
    uint64_t total_size = 0;

	size_t current_index = 0;               // index of the current file
	FILE* current_handle = nullptr;         // handle of the current file
	uint64_t logical_pos = 0;               // position in multi-file
	uint64_t pos_in_current = 0;            // position in corresponding mono-file

	// MultiFileMode mode = MultiFileMode::READ_ONLY; // for the moment , only read mode is supported
    MultiFileError error_state = MultiFileError::OK;
};
