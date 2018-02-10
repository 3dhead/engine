#include "core/tests/AbstractTest.h"
#include "io/Filesystem.h"

namespace io {

class FileTest: public core::AbstractTest {
};

TEST_F(FileTest, testGetPath) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr& file = fs.open("foobar/1.txt", io::FileMode::Read);
	ASSERT_TRUE(core::string::endsWith(file->path(), "foobar"));
	ASSERT_EQ("txt", file->extension());
	ASSERT_EQ("1", file->fileName());
	ASSERT_TRUE(core::string::endsWith(file->name(), "foobar/1.txt"));
	ASSERT_FALSE(file->exists());
}

}
