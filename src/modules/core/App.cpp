/**
 * @file
 */

#include "App.h"
#include "AppCommand.h"
#include "Var.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "io/Filesystem.h"
#include "Common.h"
#include "metric/UDPMetricSender.h"
#include "Log.h"
#include "Tokenizer.h"
#include "Concurrency.h"
#include <thread>
#include <SDL.h>
#include "engine-config.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

namespace core {

App* App::_staticInstance;
thread_local std::stack<App::TraceData> App::_traceData;

App::App(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		_filesystem(filesystem), _eventBus(eventBus), _threadPool(threadPoolSize, "Core"),
		_timeProvider(timeProvider), _metric(metric) {

	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
	_now = systemMillis();
	_timeProvider->update(_now);
	_staticInstance = this;
}

App::~App() {
	core_trace_set(nullptr);
	_metricSender->shutdown();
	_metric->shutdown();
	Log::shutdown();
}

void App::init(const std::string& organisation, const std::string& appname) {
	_organisation = organisation;
	_appname = appname;
}

int App::startMainLoop(int argc, char *argv[]) {
	_argc = argc;
	_argv = argv;

	while (AppState::InvalidAppState != _curState) {
		onFrame();
	}
	return _exitCode;
}

void App::addBlocker(AppState blockedState) {
	_blockers.insert(blockedState);
}

void App::remBlocker(AppState blockedState) {
	_blockers.erase(blockedState);
}

void App::traceBeginFrame(const char *threadName) {
}

void App::traceBegin(const char *threadName, const char* name) {
	_traceData.emplace(TraceData{threadName, name, core::TimeProvider::systemNanos()});
}

void App::traceEnd(const char *threadName) {
	if (_blockMetricsUntilNextFrame) {
		return;
	}
	core_assert(!_traceData.empty());
	TraceData traceData = _traceData.top();
	_traceData.pop();
	const double dt = core::TimeProvider::systemNanos() - traceData.nanos;
	const double dtMillis = dt * 1000.0;
	_metric->gauge(traceData.name, uint64_t(dtMillis * 1000.0), {{"thread", traceData.threadName}});
}

void App::traceEndFrame(const char *threadName) {
	if (!_blockMetricsUntilNextFrame) {
		return;
	}
	while (!_traceData.empty()) {
		_traceData.pop();
	}
	_blockMetricsUntilNextFrame = false;
}

void App::onFrame() {
	core_trace_begin_frame();
	if (_nextState != AppState::InvalidAppState && _nextState != _curState) {
		if (_blockers.find(_nextState) != _blockers.end()) {
			if (AppState::Blocked != _curState) {
				_curState = AppState::Blocked;
			}
		} else {
			_curState = _nextState;
			_nextState = AppState::InvalidAppState;
		}
	}

	if (AppState::Blocked == _curState) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} else {
		const uint64_t now = systemMillis();
		_deltaFrame = (std::max)(uint64_t(1), now - _now);
		_timeProvider->update(now);
		_now = now;

		switch (_curState) {
		case AppState::Construct: {
			core_trace_scoped(AppOnConstruct);
			_nextState = onConstruct();
			break;
		}
		case AppState::Init: {
			core_trace_scoped(AppOnInit);
			_nextState = onInit();
			_nextFrame = systemMillis();
			break;
		}
		case AppState::InitFailure: {
			core_trace_scoped(AppOnCleanup);
			_exitCode = 1;
			_nextState = onCleanup();
			break;
		}
		case AppState::Running: {
			{
				core_trace_scoped(AppOnRunning);
				if (_framesPerSecondsCap < 1.0 || _nextFrame > now) {
					{
						core_trace_scoped(AppOnBeforeRunning);
						onBeforeRunning();
					}
					const AppState state = onRunning();
					if (_nextState != AppState::Cleanup && _nextState != AppState::Destroy) {
						_nextState = state;
					}
					if (AppState::Running == _nextState) {
						core_trace_scoped(AppOnAfterRunning);
						onAfterRunning();
					}
				}
				if (_framesPerSecondsCap > 1.0) {
					const uint64_t delay = _nextFrame - now;
					if (delay > 0u) {
						std::this_thread::sleep_for(std::chrono::milliseconds(delay));
					}
					_nextFrame += (1000.0 / _framesPerSecondsCap) + 0.00001;
				}
			}
			break;
		}
		case AppState::Cleanup: {
			core_trace_scoped(AppOnCleanup);
			_nextState = onCleanup();
			break;
		}
		case AppState::Destroy: {
			core_trace_scoped(AppOnDestroy);
			_nextState = onDestroy();
			_curState = AppState::InvalidAppState;
			break;
		}
		default:
			break;
		}
	}
	core_trace_end_frame();
}

AppState App::onConstruct() {
	VarPtr logVar = core::Var::get(cfg::CoreLogLevel, SDL_LOG_PRIORITY_INFO);
	registerArg("--loglevel").setShort("-l").setDescription("Change log level from 1 (trace) to 6 (only critical)");
	const std::string& logLevelVal = getArgVal("--loglevel");
	if (!logLevelVal.empty()) {
		logVar->setVal(logLevelVal);
	}
	core::Var::get(cfg::CoreSysLog, _syslog ? "true" : "false");

	Log::init();

	core::Command::registerCommand("set", [] (const core::CmdArgs& args) {
		if (args.size() != 2) {
			return;
		}
		core::Var::get(args[0], "")->setVal(args[1]);
	}).setHelp("Set a variable name");

	core::Command::registerCommand("quit", [&] (const core::CmdArgs& args) {requestQuit();}).setHelp("Quit the application");

	core::Command::registerCommand("core_trace", [&] (const core::CmdArgs& args) {
		_blockMetricsUntilNextFrame = true;
		if (core_trace_set(this) == this) {
			core_trace_set(nullptr);
			Log::info("Deactivated statsd based tracing metrics");
		} else {
			Log::info("Activated statsd based tracing metrics");
		}
	}).setHelp("Toggle application tracing via statsd");

	AppCommand::init();

	for (int i = 0; i < _argc; ++i) {
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' && _argv[i][1] == '-')) {
			continue;
		}

		const std::string command = &_argv[i][1];
		if (command != "set") {
			continue;
		}
		std::string args;
		args.reserve(256);
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.append(_argv[i++]);
			args.append(" ");
		}
		core::executeCommands(command + " " + args);
	}

	core::Var::get(cfg::MetricFlavor, "telegraf");
	const std::string& host = core::Var::get(cfg::MetricHost, "127.0.0.1")->strVal();
	const int port = core::Var::get(cfg::MetricPort, "8125")->intVal();
	_metricSender = std::make_shared<metric::UDPMetricSender>(host, port);
	if (!_metricSender->init()) {
		Log::warn("Failed to init metric sender");
		return AppState::Destroy;;
	}
	if (!_metric->init(_appname.c_str(), _metricSender)) {
		Log::warn("Failed to init metrics");
		// no hard error...
	}

	Log::init();

	Log::debug("%s: " PROJECT_VERSION, _appname.c_str());

	for (int i = 0; i < _argc; ++i) {
		Log::debug("argv[%i] = %s", i, _argv[i]);
	}

	if (_coredump) {
#ifdef HAVE_SYS_RESOURCE_H
		struct rlimit core_limits;
		core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &core_limits);
		Log::debug("activate core dumps");
#else
		Log::debug("can't activate core dumps");
#endif
	}

	_filesystem->init(_organisation, _appname);

	return AppState::Init;
}

AppState App::onInit() {
	_initTime = _now;

	SDL_Init(SDL_INIT_TIMER|SDL_INIT_EVENTS);
	_threadPool.init();

	Log::debug("detected %u cpus", core::cpus());
	const std::string& content = _filesystem->load(_appname + ".vars");
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const std::string& name = t.next();
		if (!t.hasNext()) {
			break;
		}
		const std::string& value = t.next();
		if (!t.hasNext()) {
			break;
		}
		const std::string& flags = t.next();
		int32_t flagsMask = -1;
		uint32_t flagsMaskFromFile = 0u;
		for (char c : flags) {
			if (c == 'R') {
				flagsMaskFromFile |= CV_READONLY;
				Log::trace("read only flag for %s", name.c_str());
			} else if (c == 'S') {
				flagsMaskFromFile |= CV_SHADER;
				Log::trace("shader flag for %s", name.c_str());
			} else if (c == 'X') {
				flagsMaskFromFile |= CV_SECRET;
				Log::trace("secret flag for %s", name.c_str());
			}
		}
		const VarPtr& old = core::Var::get(name);
		if (old) {
			flagsMask = (int32_t)(flagsMaskFromFile | old->getFlags());
		} else if (flagsMaskFromFile != 0u) {
			flagsMask = (int32_t)flagsMaskFromFile;
		}
		core::Var::get(name, value.c_str(), flagsMask);
	}

	Log::init();
	Log::trace("handle %i command line arguments", _argc);
	for (int i = 0; i < _argc; ++i) {
		// every command is started with a '-'
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' &&_argv[i][1] == '-')) {
			continue;
		}

		const std::string command = &_argv[i][1];
		if (command == "set") {
			// already handled
			continue;
		}
		std::string args;
		args.reserve(256);
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.append(_argv[i++]);
			args.append(" ");
		}
		Log::trace("Execute %s with %i arguments", command.c_str(), (int)args.size());
		core::executeCommands(command + " " + args);
	}
	core::Var::visit([&] (const core::VarPtr& var) {
		var->markClean();
	});
	const std::string& autoexecCommands = filesystem()->load("autoexec.cfg");
	if (!autoexecCommands.empty()) {
		Command::execute(autoexecCommands);
	}

	// we might have changed the loglevel from the commandline
	Log::init();
	_logLevelVar = core::Var::getSafe(cfg::CoreLogLevel);
	_syslogVar = core::Var::getSafe(cfg::CoreSysLog);

	core_trace_init();

	for (int i = 0; i < _argc; ++i) {
		if (!strcmp(_argv[i], "--help") || !strcmp(_argv[i], "-h")) {
			usage();
			return AppState::Destroy;
		}
	}

	return AppState::Running;
}

void App::usage() const {
	Log::info("Usage: %s [--help] [-set configvar value] [-commandname]", _appname.c_str());

	int maxWidthLong = 0;
	int maxWidthShort = 0;
	for (const Argument& a : _arguments) {
		maxWidthLong = (std::max)(maxWidthLong, (int)a.longArg().size());
		maxWidthShort = (std::max)(maxWidthShort, (int)a.shortArg().size());
	};
	int maxWidthOnlyLong = maxWidthLong + maxWidthShort + 3;
	for (const Argument& a : _arguments) {
		const std::string defaultVal = a.defaultValue().empty() ? "" : core::string::format(" (default: %s)", a.defaultValue().c_str());
		if (a.shortArg().empty()) {
			Log::info("%-*s - %s %s", maxWidthOnlyLong,
				a.longArg().c_str(), a.description().c_str(), defaultVal.c_str());
		} else {
			Log::info("%-*s | %-*s - %s %s", maxWidthLong,
				a.longArg().c_str(), maxWidthShort, a.shortArg().c_str(), a.description().c_str(), defaultVal.c_str());
		}
	}

	int maxWidth = 0;
	core::Var::visitSorted([&] (const core::VarPtr& v) {
		maxWidth = (std::max)(maxWidth, (int)v->name().size());
	});
	core::Command::visitSorted([&] (const core::Command& c) {
		maxWidth = (std::max)(maxWidth, (int)c.name().size());
	});

	Log::info("---");
	Log::info("Config variables:");
	core::Var::visitSorted([=] (const core::VarPtr& v) {
		const uint32_t flags = v->getFlags();
		std::string flagsStr = "     ";
		const char *value = v->strVal().c_str();
		if ((flags & CV_READONLY) != 0) {
			flagsStr[0]  = 'R';
		}
		if ((flags & CV_NOPERSIST) != 0) {
			flagsStr[1]  = 'N';
		}
		if ((flags & CV_SHADER) != 0) {
			flagsStr[2]  = 'S';
		}
		if ((flags & CV_SECRET) != 0) {
			flagsStr[3]  = 'X';
			value = "***secret***";
		}
		if (v->isDirty()) {
			flagsStr[4]  = 'D';
		}
		Log::info("   %-*s %s %s", maxWidth, v->name().c_str(), flagsStr.c_str(), value);
	});
	Log::info("Flags:");
	Log::info("   %-*s Readonly  can't get modified at runtime - only at startup", maxWidth, "R");
	Log::info("   %-*s Nopersist value won't get persisted in the cfg file", maxWidth, "N");
	Log::info("   %-*s Shader    changing the value would result in a recompilation of the shaders", maxWidth, "S");
	Log::info("   %-*s Dirty     the config variable is dirty, means that the initial value was changed", maxWidth, "D");
	Log::info("   %-*s Secret    the value of the config variable won't be shown in the logs", maxWidth, "X");

	Log::info("---");
	Log::info("Commands:");
	core::Command::visitSorted([=] (const core::Command& c) {
		Log::info("   %-*s %s", maxWidth, c.name().c_str(), c.help().c_str());
	});
}

void App::onAfterRunning() {
}

void App::onBeforeRunning() {
}

AppState App::onRunning() {
	if (_logLevelVar->isDirty() || _syslogVar->isDirty()) {
		Log::init();
		_logLevelVar->markClean();
		_syslogVar->markClean();
	}

	core::Command::executeDelayed();

	_filesystem->update();

	return AppState::Cleanup;
}

bool App::hasArg(const std::string& arg) const {
	for (int i = 1; i < _argc; ++i) {
		if (arg == _argv[i]) {
			return true;
		}
	}
	return false;
}

std::string App::getArgVal(const std::string& arg, const std::string& defaultVal) {
	for (int i = 1; i < _argc; ++i) {
		if (arg != _argv[i]) {
			continue;
		}
		if (i + 1 < _argc) {
			return _argv[i + 1];
		}
	}
	if (!defaultVal.empty()) {
		return defaultVal;
	}
	for (const Argument& a : _arguments) {
		if (a.longArg() != arg && a.shortArg() != arg) {
			continue;
		}
		for (int i = 1; i < _argc; ++i) {
			if (a.longArg() != _argv[i] && a.shortArg() != _argv[i]) {
				continue;
			}
			if (i + 1 < _argc) {
				return _argv[i + 1];
			}
		}
		if (!a.mandatory()) {
			return a.defaultValue();
		}
		if (a.defaultValue().empty()) {
			usage();
			requestQuit();
		}
		return a.defaultValue();
	}
	return "";
}

App::Argument& App::registerArg(const std::string& arg) {
	App::Argument argument(arg);
	_arguments.push_back(argument);
	return _arguments.back();
}

AppState App::onCleanup() {
	if (_suspendRequested) {
		addBlocker(AppState::Init);
		return AppState::Init;
	}

	if (!_organisation.empty() && !_appname.empty()) {
		Log::debug("save the config variables");
		std::stringstream ss;
		core::Var::visitSorted([&](const core::VarPtr& var) {
			if ((var->getFlags() & core::CV_NOPERSIST) != 0u) {
				return;
			}
			const uint32_t flags = var->getFlags();
			std::string flagsStr;
			const char *value = var->strVal().c_str();
			if ((flags & CV_READONLY) == CV_READONLY) {
				flagsStr.append("R");
			}
			if ((flags & CV_SHADER) == CV_SHADER) {
				flagsStr.append("S");
			}
			if ((flags & CV_SECRET) == CV_SECRET) {
				flagsStr.append("X");
			}
			ss << R"(")" << var->name() << R"(" ")" << value << R"(" ")" << flagsStr << R"(")" << std::endl;
		});
		const std::string& str = ss.str();
		_filesystem->write(_appname + ".vars", str);
	} else {
		Log::warn("don't save the config variables");
	}

	core::Command::shutdown();
	core::Var::shutdown();

	const SDL_AssertData *item = SDL_GetAssertionReport();
	while (item != nullptr) {
		Log::warn("'%s', %s (%s:%d), triggered %u times, always ignore: %s.\n",
				item->condition, item->function, item->filename, item->linenum,
				item->trigger_count, item->always_ignore != 0 ? "yes" : "no");
		item = item->next;
	}
	SDL_ResetAssertionReport();

	_filesystem->shutdown();
	_threadPool.shutdown();

	core_trace_shutdown();

	if (_metricSender) {
		_metricSender->shutdown();
	}
	if (_metric) {
		_metric->shutdown();
	}

	return AppState::Destroy;
}

AppState App::onDestroy() {
	return AppState::InvalidAppState;
}

void App::readyForInit() {
	remBlocker(AppState::Init);
}

void App::requestQuit() {
	if (AppState::Running == _curState) {
		_nextState = AppState::Cleanup;
	} else {
		_nextState = AppState::Destroy;
	}
}

void App::requestSuspend() {
	_nextState = AppState::Cleanup;
	_suspendRequested = true;
}

const std::string& App::currentWorkingDir() const {
	return _filesystem->basePath();
}

}
