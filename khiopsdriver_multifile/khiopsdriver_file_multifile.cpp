// Pour eviter les warning sur strerror
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "khiopsdriver_file_multifile.h"
#include "multifile.h"
#include "globbing.h"

#if defined(__linux__) || defined(__APPLE__)
#define __linux_or_apple__
#endif

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <iostream>

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#include <windows.h>
#endif // _MSC_VER

#ifdef __linux_or_apple__
#include <unistd.h>
#ifdef __gnu_linux__
#include <sys/vfs.h> // ANDROID https://svn.boost.org/trac/boost/ticket/8816
#else
#include <sys/statvfs.h>
#endif // __clang__
#endif // __linux_or_apple__

#define ERROR_ON_NULL_ARG(arg, err_val)                                                                        \
	if (!(arg))                                                                                                \
	{                                                                                                          \
		LogError("Null arg error");                                                                            \
		return (err_val);                                                                                      \
	}

using namespace std;

// Define to compile a read-only version of the driver
// Uncomment the following line to compile the read-only version of the driver
// #define __parquetreadonlydriver__

static thread_local const char* g_lastError;

void LogError(const char* msg) {
	g_lastError = std::move(msg);
}

const char* driver_getDriverName() {
	return "Multi-file driver";
}

const char* driver_getVersion() {
	return "0.0.1";
}

const char* driver_getScheme() {
	return "multifile";
}

int driver_isReadOnly() {
	return 1;
}

int driver_connect() {
	return 1;
}

int driver_disconnect() {
	return 1;
}

int driver_isConnected() {
	return 1;
}

// Nombre de caracteres du nom du scheme
int getSchemeCharNumber() {
	static const int nSchemeCharNumber = (int)strlen(driver_getScheme());
	return nSchemeCharNumber;
}

// Test si un fichier est gere par le scheme
int isManaged(const char* sFilePathName) {
	int ok;

	assert(sFilePathName != NULL);

	// Le debut du nom de fichier doit etre de la forme 'scheme://' ou 'scheme:///'
	ok = strncmp(sFilePathName, driver_getScheme(), getSchemeCharNumber()) == 0;
	ok = ok && sFilePathName[getSchemeCharNumber()] == ':';
	ok = ok && sFilePathName[getSchemeCharNumber() + 1] == '/';
	ok = ok && sFilePathName[getSchemeCharNumber() + 2] == '/';
	return ok;
}

// Methode utilitaire pour avoir acces au nom du fichier sans son schema
const char* getFilePath(const char* sFilePathName) {
	int nStartFilePath;

	// La gestion du nombre de '/' n'est pas claire
	// Selon https://en.wikipedia.org/wiki/File_URI_scheme , on peut avoir de 1 a 4 '/' selon les cas
	// Des tests sous windows avec un navigateur firefox ou chrome montrent un grande tolerance au nombre de '/'.
	// Pourvue que le nom commence par 'file:', on peut avoir un nombre quelconque de '/', meme zero.
	// Firefox le corrige en mettant 'file:///'<path>, et chrone en omettant le scheme et en gardant juste <path>
	// On decide ici d'appliquer une politique souple, avec un nombre quelconque de '/', au moins un sous linux.

	// On extrait le chemin du fichier si le schema est correct
	if (isManaged(sFilePathName))
	{
		// Le debut du nom de fichier doit etre de la forme 'scheme:' suivi d'un nombre quelconque de '/'
		// Sous windows, on se place on premier caractere non '/', et sous linux, on inclus le dernier '/'
		// On renvoie un path commencant par '/'
		nStartFilePath = getSchemeCharNumber() + 1;
		while (sFilePathName[nStartFilePath] == '/')
			nStartFilePath++;
		assert(sFilePathName[nStartFilePath] != '/');
#ifndef _MSC_VER
		nStartFilePath--;
		assert(sFilePathName[nStartFilePath] == '/');
#endif // _MSC_VER
		return &sFilePathName[nStartFilePath];
	}
	// Sinon, on renvoie le nom du fichier tel quel
	else
		return sFilePathName;
}

int driver_fileExists(const char* filename) {
	ERROR_ON_NULL_ARG(filename, 0);
	const char* globbing_path = getFilePath(filename); // just to check the schema is correct

	char* valid_path = (char*)malloc((strlen(globbing_path) + 2) * sizeof(char));
	if (valid_path == NULL) {
		LogError("driver_getFileSize: unable to malloc to add \':\' to path");
		return 0;
	}
	valid_path[0] = globbing_path[0];
	valid_path[1] = ':';
	for (size_t i = 1; i <= strlen(globbing_path); i++)
		valid_path[i + 1] = globbing_path[i];

	valid_path[strlen(globbing_path) + 1] = '\0';

	if (strchr(valid_path, '*') == nullptr) {
		// add wildcard before the file extension
		char* dot = strrchr(valid_path, '.');
		if (dot != nullptr) {
			size_t base_length = dot - valid_path;
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strncpy(new_path, valid_path, base_length);
			new_path[base_length] = '*';
			strcpy(&new_path[base_length + 1], dot);
			free(valid_path);
			valid_path = new_path;
		}
		else {
			// no extension, just add a '*'
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strcpy(new_path, valid_path);
			new_path[strlen(valid_path)] = '*';
			new_path[strlen(valid_path) + 1] = '\0';
			free(valid_path);
			valid_path = new_path;
		}
		// std::cout << "Corrected path to add wildcard: " << valid_path << std::endl;
	}

	vector<string> filenames = get_matching_files(valid_path);
	free(valid_path);
	if (filenames.size() == 0) {
		return 0;
	}
	return 1;
}

int driver_dirExists(const char* filename) {
	int bIsDirectory = false;

#ifdef _WIN32
	boolean bExist;

	bExist = _access(getFilePath(filename), 0) != -1;
	if (bExist)
	{
		// On test si ca n'est pas un fichier, car sous Windows, la racine ("C:") existe mais n'est
		// consideree par l'API _stat64 ni comme une fichier ni comme un repertoire
		boolean bIsFile = false;
		struct __stat64 fileStat;
		if (_stat64(filename, &fileStat) == 0)
			bIsFile = ((fileStat.st_mode & S_IFMT) == S_IFREG);
		bIsDirectory = !bIsFile;
	}
#else // _WIN32

	struct stat s;
	if (stat(getFilePath(filename), &s) == 0)
		bIsDirectory = ((s.st_mode & S_IFMT) == S_IFDIR);

#endif // _WIN32

	return bIsDirectory;
}

long long int driver_getSingleFileSize(const char* filename) {
	long long int filesize;
	int nError;

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#if defined _WIN32
	struct __stat64 fileStat;
	nError = _stat64(getFilePath(filename), &fileStat);
#elif defined(__APPLE__)
	struct stat fileStat;
	nError = stat(getFilePath(filename), &fileStat);
#elif defined(__linux__)
	struct stat64 fileStat;
	nError = stat64(filename, &fileStat);
#elif
	nError = 1; // undefined in the current OS
#endif
	if (nError != 0)
		filesize = 0;
	else
		filesize = fileStat.st_size;

	return filesize;
}

long long int driver_getFileSize(const char* filename) {
	ERROR_ON_NULL_ARG(filename, -1);
	const char* globbing_path = getFilePath(filename); // just to check the schema is correct

	char* valid_path = (char*)malloc((strlen(globbing_path) + 2) * sizeof(char));
	if (valid_path == NULL) {
		LogError("driver_getFileSize: unable to malloc to add \':\' to path");
		return -1;
	}
	valid_path[0] = globbing_path[0];
	valid_path[1] = ':';
	for (size_t i = 1; i <= strlen(globbing_path); i++)
		valid_path[i + 1] = globbing_path[i];

	valid_path[strlen(globbing_path) + 1] = '\0';

	if (strchr(valid_path, '*') == nullptr) {
		// add wildcard before the file extension
		char* dot = strrchr(valid_path, '.');
		if (dot != nullptr) {
			size_t base_length = dot - valid_path;
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strncpy(new_path, valid_path, base_length);
			new_path[base_length] = '*';
			strcpy(&new_path[base_length + 1], dot);
			free(valid_path);
			valid_path = new_path;
		}
		else {
			// no extension, just add a '*'
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strcpy(new_path, valid_path);
			new_path[strlen(valid_path)] = '*';
			new_path[strlen(valid_path) + 1] = '\0';
			free(valid_path);
			valid_path = new_path;
		}
		// std::cout << "Corrected path to add wildcard: " << valid_path << std::endl;
	}

	vector<string> filenames = get_matching_files(valid_path);
	if (filenames.size() == 0) {
		LogError("driver_getFileSize: no files found after finding matching files");
		return -1;
	}

	free(valid_path);

	int res = 0;

	for (std::string filename : filenames) {
		long long int filesize = driver_getSingleFileSize(filename.c_str());
		if (filesize < 0) {
			LogError("driver_fopen: couldn't found a file size");
			return -1;
		}
		res += filesize;
	}

	return res;
}

void* driver_fopen(const char* filename, char mode) {
	ERROR_ON_NULL_ARG(filename, nullptr);
	if (mode != 'r') {
		LogError("Only read mode is supported");
		return NULL;
	}

	const char* globbing_path = getFilePath(filename); // just to check the schema is correct

	char* valid_path = (char*)malloc((strlen(globbing_path) + 2) * sizeof(char));
	if (valid_path == NULL) {
		LogError("driver_getFileSize: unable to malloc to add \':\' to path");
		return NULL;
	}

	valid_path[0] = globbing_path[0];
	valid_path[1] = ':';
	for (size_t i = 1; i <= strlen(globbing_path); i++)
		valid_path[i + 1] = globbing_path[i];

	valid_path[strlen(globbing_path) + 1] = '\0';

	if (strchr(valid_path, '*') == nullptr) {
		// add wildcard before the file extension
		char* dot = strrchr(valid_path, '.');
		if (dot != nullptr) {
			size_t base_length = dot - valid_path;
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strncpy(new_path, valid_path, base_length);
			new_path[base_length] = '*';
			strcpy(&new_path[base_length + 1], dot);
			free(valid_path);
			valid_path = new_path;
		}
		else {
			// no extension, just add a '*'
			char* new_path = (char*)malloc((strlen(valid_path) + 2) * sizeof(char));
			if (new_path == NULL) {
				free(valid_path);
				LogError("driver_fopen: unable to malloc to add wildcard to path");
				return NULL;
			}
			strcpy(new_path, valid_path);
			new_path[strlen(valid_path)] = '*';
			new_path[strlen(valid_path) + 1] = '\0';
			free(valid_path);
			valid_path = new_path;
		}
		// std::cout << "Corrected path to add wildcard: " << valid_path << std::endl;
	}

	vector<string> filenames = get_matching_files(valid_path);
	free(valid_path);
	if (filenames.size() == 0) {
		LogError("driver_fopen: no files found after finding matching files");
		return NULL;
	}

	MultiFile* mf = new MultiFile();
	mf->filenames = filenames;
	for (std::string filename : filenames) {
		long long int filesize = driver_getSingleFileSize(filename.c_str());
		if (filesize < 0) {
			delete mf;
			LogError("driver_fopen: couldn't found a file size");
			return NULL;
		}
		mf->file_sizes.push_back(filesize);
		mf->prefix_offsets.push_back(mf->total_size);
		mf->total_size += filesize;
	}

	FILE* handle = std::fopen(getFilePath(filenames[0].c_str()), "rb");
	if (handle == nullptr) {
		delete mf;
		LogError("driver_fopen: couldn't open first file");
		return NULL;
	}
	mf->current_handle = handle;

	return mf;
}

int driver_fclose(void* multifile_ptr) {
	ERROR_ON_NULL_ARG(multifile_ptr, EOF);

	MultiFile* multifile = static_cast<MultiFile*>(multifile_ptr);

	int code = EOF;
	if (multifile->current_handle != nullptr) {
		code = std::fclose(multifile->current_handle);
		multifile->current_handle = nullptr;
	}

	delete multifile;
	return code;
}

long long int driver_fread(void* ptr, size_t size, size_t count, void* multifile_ptr) {
	ERROR_ON_NULL_ARG(ptr, -1);
	ERROR_ON_NULL_ARG(multifile_ptr, -1);

	MultiFile* multifile = static_cast<MultiFile*>(multifile_ptr);
	if (multifile->error_state != MultiFileError::OK) {
		return -1;
	}

	// std::cout << "driver_fread: read " << size << " * " << count << " in multifile file (" << multifile->filepath << ")" << std::endl;


	size_t total_bytes_to_read = size * count;
	size_t total_bytes_read = 0;
	if (total_bytes_to_read == 0) {
		return 0;
	}
	while (total_bytes_read < total_bytes_to_read) {
		if (multifile->current_handle == nullptr) {
			multifile->error_state = MultiFileError::IO_ERROR;
			LogError("No file opened ready to read");
			break;
		}

		size_t bytes_left_in_current = multifile->file_sizes[multifile->current_index] - multifile->pos_in_current;
		size_t bytes_to_read_now = min(total_bytes_to_read - total_bytes_read, bytes_left_in_current);

		size_t bytes_read = std::fread(static_cast<char*>(ptr) + total_bytes_read, 1, bytes_to_read_now, multifile->current_handle);

		total_bytes_read += bytes_read;
		multifile->logical_pos += bytes_read;
		multifile->pos_in_current += bytes_read;

		if (std::ftell(multifile->current_handle) != multifile->pos_in_current) {
			printf("desync! ftell=%ld, pos_in_current=%zu\n", std::ftell(multifile->current_handle), multifile->pos_in_current);
		}


		if (bytes_read == bytes_left_in_current) {
			if (multifile->current_index + 1 < multifile->filenames.size()) {
				// Passer au fichier suivant
				std::fclose(multifile->current_handle);
				multifile->current_handle = nullptr;

				multifile->current_index++;
				multifile->pos_in_current = 0;
				multifile->current_handle = std::fopen(getFilePath(multifile->filenames[multifile->current_index].c_str()), "rb");

				if (multifile->current_handle == nullptr) {
					multifile->error_state = MultiFileError::OPEN_FAILED;
					LogError("Couldn't open next file");
					break;
				}
				continue; // Reprendre la lecture sur le prochain fichier
			}
			else {
				// Plus de fichiers à lire
				break;
			}
		}
	}
	return total_bytes_read / size;
}

int driver_fseek(void* multifile, long long int offset, int whence) {
	ERROR_ON_NULL_ARG(multifile, -1);

	MultiFile* mf = static_cast<MultiFile*>(multifile);
	if (mf->error_state != MultiFileError::OK) {
		return -1;
	}
	std::string whence_str = (whence == std::ios::beg) ? "BEGIN" : (whence == std::ios::cur) ? "CURRENT" : (whence == std::ios::end) ? "END" : "UNKNOWN";

	long long int new_logical_pos;
	switch (whence) {
	case std::ios::beg:
		new_logical_pos = offset;
		break;
	case std::ios::cur:
		new_logical_pos = mf->logical_pos + offset;
		break;
	case std::ios::end:
		new_logical_pos = mf->total_size + offset;
		break;
	default:
		mf->error_state = MultiFileError::SEEK_ERROR;
		LogError("Invalid whence");
		return -1;
	}
	if (new_logical_pos < 0 || new_logical_pos > mf->total_size) {
		mf->error_state = MultiFileError::SEEK_ERROR;
		LogError("new postion out of bound");
		return -1;
	}
	// Find the file corresponding to the new logical position
	size_t new_index = 0;

	// binary search could be used here for efficiency
	while (new_index < mf->filenames.size() && mf->prefix_offsets[new_index] + mf->file_sizes[new_index] < new_logical_pos) {
		new_index++;
	}
	if (new_index >= mf->filenames.size()) {
		mf->error_state = MultiFileError::SEEK_ERROR;
		LogError("Couldn't find the right file according to offset");
		return -1;
	}


	if (new_index != mf->current_index) {
		if (mf->current_handle != nullptr) {
			std::fclose(mf->current_handle);
			mf->current_handle = nullptr;
		}
		mf->current_handle = std::fopen(getFilePath(mf->filenames[new_index].c_str()), "rb");
		if (mf->current_handle == nullptr) {
			mf->error_state = MultiFileError::OPEN_FAILED;
			LogError("Couldn't open the right file accroding to offset");
			return -1;
		}
		mf->current_index = new_index;
	}
	long long int pos_in_current = new_logical_pos - mf->prefix_offsets[new_index];
	if (std::fseek(mf->current_handle, static_cast<long>(pos_in_current), SEEK_SET) != 0) {
		mf->error_state = MultiFileError::SEEK_ERROR;
		return -1;
	}
	mf->logical_pos = new_logical_pos;
	mf->pos_in_current = pos_in_current;
	return 0;
}

const char* driver_getlasterror() {
	return g_lastError;
}