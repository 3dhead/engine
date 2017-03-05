/**
 * @file
 */

#pragma once

#include <string>
#include <memory>
#include "IOResource.h"

struct SDL_RWops;

namespace io {

enum class FileMode {
	Read, Write
};

extern void normalizePath(std::string& str);

/**
 * @brief Wrapper for file based io.
 *
 * @see FileSystem
 */
class File : public IOResource {
	friend class FileStream;
protected:
	SDL_RWops* _file;
	std::string _rawPath;
	FileMode _mode;

	void close();
	int read(void *buf, size_t size, size_t maxnum);
	long tell() const;
	long seek(long offset, int seekType) const;

public:
	File(const std::string& rawPath, FileMode mode);
	virtual ~File();

	/**
	 * @return The FileMode the file was opened with
	 */
	FileMode mode();

	bool exists() const;
	/**
	 * @return -1 on error, otherwise the length of the file
	 */
	long length() const;
	/**
	 * @return The extension of the file - or en ampty string
	 * if no extension was found
	 */
	std::string extension() const;
	/**
	 * @return The path of the file, without the name - or an
	 * empty string if no path component was found
	 */
	std::string path() const;
	/**
	 * @return Just the base file name component part - without
	 * path and extension
	 */
	std::string fileName() const;
	/**
	 * @return The full raw path of the file
	 */
	const std::string& name() const;

	SDL_RWops* createRWops(FileMode mode) const;
	long write(const unsigned char *buf, size_t len) const;
	int read(void **buffer);
	int read(void *buffer, int n);
	std::string load();
};

inline FileMode File::mode() {
	return _mode;
}

typedef std::shared_ptr<File> FilePtr;

}
