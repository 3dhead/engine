#include "testcore/TestMeshApp.h"
#include "io/Filesystem.h"

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestMeshApp app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
