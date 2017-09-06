#include "core/tests/AbstractTest.h"
#include "io/Filesystem.h"
#include <algorithm>

namespace io {

class FilesystemTest: public core::AbstractTest {
};

::std::ostream& operator<<(::std::ostream& ostream, const std::vector<io::Filesystem::DirEntry>& val) {
	for (const auto& e : val) {
		ostream << e.name << " - " << std::enum_value(e.type) << ", ";
	}
	return ostream;
}

TEST_F(FilesystemTest, testListDirectory) {
	io::Filesystem fs;
	fs.init("test", "test");
	ASSERT_TRUE(fs.createDir("listdirtest/dir1"));
	ASSERT_TRUE(fs.syswrite("listdirtest/dir1/ignored", "ignore"));
	ASSERT_TRUE(fs.syswrite("listdirtest/dir1/ignoredtoo", "ignore"));
	ASSERT_TRUE(fs.syswrite("listdirtest/file1", "1"));
	ASSERT_TRUE(fs.syswrite("listdirtest/file2", "2"));
	std::vector<io::Filesystem::DirEntry> entities;
	fs.list("listdirtest/", entities, "");
	ASSERT_FALSE(entities.empty());
	ASSERT_EQ(3u, entities.size()) << entities;
	std::sort(entities.begin(), entities.end(),
		[] (const io::Filesystem::DirEntry& first, const io::Filesystem::DirEntry& second) {
			return first.name < second.name;
		});
	ASSERT_EQ("dir1", entities[0].name);
	ASSERT_EQ("file1", entities[1].name);
	ASSERT_EQ("file2", entities[2].name);
	ASSERT_EQ(io::Filesystem::DirEntry::Type::dir, entities[0].type);
	ASSERT_EQ(io::Filesystem::DirEntry::Type::file, entities[1].type);
	ASSERT_EQ(io::Filesystem::DirEntry::Type::file, entities[2].type);
	fs.shutdown();
}

TEST_F(FilesystemTest, testListFilter) {
	io::Filesystem fs;
	fs.init("test", "test");
	ASSERT_TRUE(fs.createDir("listdirtestfilter"));
	ASSERT_TRUE(fs.createDir("listdirtestfilter/dirxyz"));
	ASSERT_TRUE(fs.syswrite("listdirtestfilter/filexyz", "1"));
	ASSERT_TRUE(fs.syswrite("listdirtestfilter/fileother", "2"));
	ASSERT_TRUE(fs.syswrite("listdirtestfilter/fileignore", "3"));
	std::vector<io::Filesystem::DirEntry> entities;
	fs.list("listdirtestfilter/", entities, "*xyz");
	ASSERT_EQ(2u, entities.size()) << entities;
	fs.shutdown();
}

TEST_F(FilesystemTest, testMkdir) {
	io::Filesystem fs;
	fs.init("test", "test");
	ASSERT_TRUE(fs.createDir("testdir"));
	ASSERT_TRUE(fs.createDir("testdir2/subdir/other"));
	fs.shutdown();
}

}
