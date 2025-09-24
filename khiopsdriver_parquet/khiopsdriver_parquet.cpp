// Pour eviter les warning sur strerror
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "khiopsdriver_parquet.h"
#include "file_finder.h"
#include "parquet_to_csv.h"

#if defined(__linux__) || defined(__APPLE__)
#define __linux_or_apple__
#endif

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

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

// Define to compile a read-only version of the driver
// Uncomment the following line to compile the read-only version of the driver
// #define __parquetreadonlydriver__

const char* driver_getDriverName()
{
	return "Parquet driver";
}

const char* driver_getVersion()
{
	return "1.0.0";
}

const char* driver_getScheme()
{
#ifdef __parquetreadonlydriver__
	return "parquetro";
#else
	return "parquet";
#endif 
}

int driver_isReadOnly()
{
#ifdef __parquetreadonlydriver__
	return 1;
#else
	return 0;
#endif 
}

int driver_connect()
{
	return 0;
}

int driver_disconnect()
{
	return 1;
}

int driver_isConnected()
{
	return 1;
}

// Nombre de caracteres du nom du scheme
int getSchemeCharNumber()
{
	static const int nSchemeCharNumber = (int)strlen(driver_getScheme());
	return nSchemeCharNumber;
}

// Test si un fichier est gere par le scheme
int isManaged(const char* sFilePathName)
{
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
const char* getFilePath(const char* sFilePathName)
{
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

int driver_fileExists(const char* filename)
{
	int bIsFile = false;

#ifdef _WIN32
	struct __stat64 fileStat;
	if (_stat64(getFilePath(filename), &fileStat) == 0)
		bIsFile = ((fileStat.st_mode & S_IFMT) == S_IFREG);
#else
	struct stat s;
	if (stat(getFilePath(filename), &s) == 0)
		bIsFile = ((s.st_mode & S_IFMT) == S_IFREG);
#endif // _WIN32

	return bIsFile;
}

int driver_dirExists(const char* filename)
{
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

long long int driver_getFileSize(MultiFile* multifile)
{
	if (multifile == nullptr)
		return -1;
	return multifile->total_size;
}

long long int driver_getSingleFileSize(std::string filename)
{
	long long int filesize;
	int nError;

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#if defined _WIN32
	struct __stat64 fileStat;
	nError = _stat64(getFilePath(filename.c_str()), &fileStat);
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

//MultiFile* driver_fopen_glob(const char* globbing, char mode) {
//	std::vector<const char*> filenames = get_matching_files(globbing);
//	return driver_fopen(filenames, mode);
//}

MultiFile* driver_fopen(const char* parquet, char mode) {
	std::vector<std::string> filenames = parquetToCsv(parquet);
	MultiFile* multifile = new MultiFile();

	if (filenames.size() == 0) {
		multifile->error_state = MultiFileError::NO_FILES;
		return multifile;
	}

	// Mode handling (read-only, write, append)
	/*if (mode != 'r' && mode != 'w' && mode != 'a') {
		multifile->error_state = MultiFileError::OPEN_FAILED;
		return multifile;
	}
	multifile->mode = (mode == 'r') ? MultiFileMode::READ_ONLY : MultiFileMode::READ_WRITE;*/

	multifile->filenames = filenames;

	for (std::string filename : filenames) {
		long long int filesize = driver_getSingleFileSize(filename);
		if (filesize < 0) {
			multifile->error_state = MultiFileError::OPEN_FAILED;
			return multifile;
		}
		multifile->file_sizes.push_back(filesize);
		multifile->prefix_offsets.push_back(multifile->total_size);
		multifile->total_size += filesize;
	}

	FILE* handle = std::fopen(getFilePath(filenames[0].c_str()), "rb");
	if (handle == nullptr) {
		multifile->error_state = MultiFileError::OPEN_FAILED;
		return multifile;
	}
	multifile->current_handle = handle;

	return multifile;
}

int driver_fclose(void* multifile_ptr) {
	if (multifile_ptr == nullptr) {
		return EOF;
	}
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
	if (multifile_ptr == nullptr || ptr == nullptr || size == 0 || count == 0) {
		return -1;
	}
	MultiFile* multifile = static_cast<MultiFile*>(multifile_ptr);
	if (multifile->error_state != MultiFileError::OK) {
		return -1;
	}
	size_t total_bytes_to_read = size * count;
	size_t total_bytes_read = 0;
	while (total_bytes_read < total_bytes_to_read) {
		if (multifile->current_handle == nullptr) {
			multifile->error_state = MultiFileError::IO_ERROR;
			break;
		}
		size_t bytes_left_in_current = multifile->file_sizes[multifile->current_index] - multifile->pos_in_current;
		size_t bytes_to_read_now = min(total_bytes_to_read - total_bytes_read, bytes_left_in_current);
		size_t bytes_read = std::fread(static_cast<char*>(ptr) + total_bytes_read, 1, bytes_to_read_now, multifile->current_handle);
		total_bytes_read += bytes_read;
		multifile->logical_pos += bytes_read;
		multifile->pos_in_current += bytes_read;
		if (bytes_read == bytes_left_in_current) {
			// End of current file reached
			if (multifile->current_index + 1 < multifile->filenames.size()) {
				std::fclose(multifile->current_handle);
				multifile->current_index++;
				multifile->pos_in_current = 0;
				multifile->current_handle = std::fopen(getFilePath(multifile->filenames[multifile->current_index].c_str()), "rb");
				if (multifile->current_handle == nullptr) {
					multifile->error_state = MultiFileError::OPEN_FAILED;
					break;
				}
			}
			else {
				// No more files to read
				break;
			}
		}
	}
	return total_bytes_read / size;
}

int driver_fseek(void* multifile, long long int offset, MultiFileWhence whence) {
	if (multifile == nullptr) {
		return -1;
	}
	MultiFile* mf = static_cast<MultiFile*>(multifile);
	if (mf->error_state != MultiFileError::OK) {
		return -1;
	}
	long long int new_logical_pos;
	switch (whence) {
		case MultiFileWhence::BEGIN:
			new_logical_pos = offset;
			break;
		case MultiFileWhence::CURRENT:
			new_logical_pos = mf->logical_pos + offset;
			break;
		case MultiFileWhence::END:
			new_logical_pos = mf->total_size + offset;
			break;
		default:
			mf->error_state = MultiFileError::SEEK_ERROR;
			return -1;
	}
	if (new_logical_pos < 0 || new_logical_pos > mf->total_size) {
		mf->error_state = MultiFileError::SEEK_ERROR;
		return -1;
	}
	// Find the file corresponding to the new logical position
	size_t new_index = 0;

	// binary search could be used here for efficiency
	while (new_index < mf->filenames.size() && mf->prefix_offsets[new_index] + mf->file_sizes[new_index] <= new_logical_pos) {
		new_index++;
	}
	if (new_index >= mf->filenames.size()) {
		mf->error_state = MultiFileError::SEEK_ERROR;
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
	return strerror(errno);
}