/**
 * @file
 */

#include "String.h"

namespace core {
namespace string {

std::string format(const char *msg, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 1024;
	char text[bufSize];

	va_start(ap, msg);
	SDL_vsnprintf(text, bufSize, msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	return std::string(text);
}

std::string replaceAll(const std::string& str, const std::string& searchStr, const char* replaceStr, size_t replaceStrSize) {
	if (str.empty()) {
		return str;
	}
	std::string sNew = str;
	std::string::size_type loc;
	const std::string::size_type searchLength = searchStr.length();
	std::string::size_type lastPosition = 0;
	while (std::string::npos != (loc = sNew.find(searchStr, lastPosition))) {
		sNew.replace(loc, searchLength, replaceStr);
		lastPosition = loc + replaceStrSize;
	}
	return sNew;
}

void splitString(const std::string& string, std::vector<std::string>& tokens, const std::string& delimiters) {
	// Skip delimiters at beginning.
	std::string::size_type lastPos = string.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = string.find_first_of(delimiters, lastPos);

	while (std::string::npos != pos || std::string::npos != lastPos) {
		// Found a token, add it to the vector.
		tokens.push_back(string.substr(lastPos, pos - lastPos));
		// Skip delimiters. Note the "not_of"
		lastPos = string.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = string.find_first_of(delimiters, lastPos);
	}
}

std::string toLower(const std::string& string) {
	std::string convert = string;
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) std::tolower);
	return convert;
}

std::string toLower(const char* string) {
	std::string convert(string);
	std::transform(convert.begin(), convert.end(), convert.begin(), (int (*)(int)) std::tolower);
	return convert;
}

static bool patternMatch(const char *pattern, const char *text);
static bool patternMatchMulti(const char* pattern, const char* text) {
	const char *p = pattern;
	const char *t = text;
	char c;

	for (;;) {
		c = *p++;
		if (c != '?' && c != '*') {
			break;
		}
		if (*t++ == '\0' && c == '?') {
			return false;
		}
	}

	if (c == '\0') {
		return true;
	}

	const int l = SDL_strlen(t);
	for (int i = 0; i < l; ++i) {
		if (*t == c && patternMatch(p - 1, t)) {
			return true;
		}
		if (*t++ == '\0') {
			return false;
		}
	}
	return false;
}

static bool patternMatch(const char *pattern, const char *text) {
	const char *p = pattern;
	const char *t = text;
	char c;

	while ((c = *p++) != '\0') {
		switch (c) {
		case '*':
			return patternMatchMulti(p, t);
		case '?':
			if (*t == '\0') {
				return false;
			}
			++t;
			break;
		default:
			if (c != *t++) {
				return false;
			}
			break;
		}
	}
	return *t == '\0';
}

bool matches(const std::string& pattern, const char* text) {
	if (pattern.empty()) {
		return true;
	}
	return patternMatch(pattern.c_str(), text);
}

std::string concat(std::string_view first, std::string_view second) {
	std::string target;
	target.reserve(first.size() + second.size());
	target.append(first.data(), first.size());
	target.append(second.data(), second.size());
	return target;
}

static void camelCase(std::string& str, bool upperCamelCase) {
	if (str.empty()) {
		return;
	}

	size_t startIndex = str.find_first_not_of("_", 0);
	if (startIndex == std::string::npos) {
		str = "";
		return;
	}
	if (startIndex > 0) {
		str = str.substr(startIndex);
	}
	std::string::size_type pos = str.find_first_of("_", 0);
	while (std::string::npos != pos) {
		std::string sub = str.substr(0, pos);
		std::string second = str.substr(pos + 1, str.length() - (pos + 1));
		if (!second.empty()) {
			second[0] = SDL_toupper(second[0]);
			sub.append(second);
		}
		str = sub;
		if (str.empty()) {
			return;
		}
		pos = str.find_first_of("_", pos);
	}
	if (str.empty()) {
		return;
	}
	if (!upperCamelCase) {
		str[0] = SDL_tolower(str[0]);
	} else {
		str[0] = SDL_toupper(str[0]);
	}
}

std::string lowerCamelCase(const std::string& str) {
	std::string copy = str;
	lowerCamelCase(copy);
	return copy;
}

std::string upperCamelCase(const std::string& str) {
	std::string copy = str;
	upperCamelCase(copy);
	return copy;
}

void upperCamelCase(std::string& str) {
	camelCase(str, true);
}

void lowerCamelCase(std::string& str) {
	camelCase(str, false);
}

char* append(char* buf, size_t bufsize, const char* string) {
	const size_t bufl = strlen(buf);
	if (bufl >= bufsize) {
		return nullptr;
	}
	const size_t remaining = bufsize - bufl;
	if (remaining <= 1u) {
		return nullptr;
	}
	const size_t l = strlen(string);
	if (remaining <= l) {
		return nullptr;
	}
	char* p = buf + bufl;
	for (size_t i = 0u; i < l; ++i) {
		*p++ = *string++;
	}
	*p = '\0';
	return p;
}

}
}
