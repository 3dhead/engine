/**
 * @file
 */

#include "File.h"
#include <SDL.h>

namespace io {

File::File(const std::string& rawPath, FileMode mode) :
		IOResource(), _rawPath(rawPath), _mode(mode) {
	_file = createRWops(mode);
}

File::~File() {
	close();
}

bool File::exists() const {
	if (_mode == FileMode::Read) {
		return _file != nullptr;
	}

	// try to open in read mode
	SDL_RWops* ops = createRWops(FileMode::Read);
	if (ops != nullptr) {
		SDL_RWclose(ops);
		return true;
	}
	return false;
}

const std::string& File::name() const {
	return _rawPath;
}

std::string File::load() {
	char *includeBuffer;
	const int includeLen = read((void **) &includeBuffer);
	std::unique_ptr<char[]> p(includeBuffer);
	if (includeBuffer == nullptr || includeLen <= 0) {
		return "";
	}
	return std::string(includeBuffer, includeLen);
}

SDL_RWops* File::createRWops(FileMode mode) const {
	const char *fmode = "rb";
	if (mode == FileMode::Write) {
		fmode = "wb";
	}
	SDL_RWops *rwops = SDL_RWFromFile(_rawPath.c_str(), fmode);
	return rwops;
}

long File::write(const unsigned char *buf, size_t len) const {
	if (_mode != FileMode::Write) {
		return -1L;
	}

	int remaining = len;
	while (remaining != 0) {
		const size_t written = SDL_RWwrite(_file, buf, 1, remaining);
		if (written == 0) {
			return -1L;
		}

		remaining -= written;
		buf += written;
	}

	return len;
}

std::string File::path() const {
	const std::string& name = this->name();
	const size_t pos = name.rfind('/');
	if (pos == std::string::npos) {
		return "";
	}
	return name.substr(0, pos);
}

std::string File::fileName() const {
	std::string name = this->name();
	const size_t pathPos = name.rfind('/');
	if (pathPos != std::string::npos) {
		name = name.substr(pathPos + 1);
	}
	const size_t extPos = name.rfind('.');
	if (extPos != std::string::npos) {
		name = name.substr(0, extPos);
	}
	return name;
}

std::string File::extension() const {
	const char *ext = SDL_strrchr(name().c_str(), '.');
	if (ext == nullptr) {
		return "";
	}
	++ext;
	return std::string(ext);
}

long File::length() const {
	if (!exists()) {
		return -1;
	}

	const long pos = tell();
	seek(0, RW_SEEK_END);
	const long end = tell();
	seek(pos, RW_SEEK_SET);
	return end;
}

int File::read(void **buffer) {
	*buffer = nullptr;
	const long len = length();
	if (len <= 0) {
		return len;
	}
	*buffer = new char[len];
	return read(*buffer, len);
}

int File::read(void *buffer, int n) {
	const size_t blockSize = 0x10000;
	unsigned char *buf;
	size_t remaining, len;

	len = remaining = n;
	buf = (unsigned char *) buffer;

	seek(0, RW_SEEK_SET);

	while (remaining != 0u) {
		size_t block = remaining;
		if (block > blockSize) {
			block = blockSize;
		}
		const int readAmount = read(buf, 1, block);

		/* end of file reached */
		if (readAmount == 0) {
			return (len - remaining + readAmount);
		} else if (readAmount == -1) {
			return -1;
		}

		/* do some progress bar thing here... */
		remaining -= readAmount;
		buf += readAmount;
	}
	return len;
}

int File::read(void *buf, size_t size, size_t maxnum) {
	if (_mode != FileMode::Read) {
		_state = IOSTATE_FAILED;
		return -1;
	}
	const int n = SDL_RWread(_file, buf, size, maxnum);
	if (n == 0) {
		_state = IOSTATE_LOADED;
	} else if (n == -1) {
		_state = IOSTATE_FAILED;
	}
	return n;
}

void File::close() {
	if (_file != nullptr) {
		SDL_RWclose(_file);
		_file = nullptr;
	}
}

long File::tell() const {
	if (_file != nullptr) {
		return SDL_RWtell(_file);
	}
	return -1L;
}

long File::seek(long offset, int seekType) const {
	if (_file != nullptr) {
		return SDL_RWseek(_file, offset, seekType);
	}
	return -1L;
}

}
