#ifdef unix
#undef unix
#endif

#include <map>
#include <mesos/mesos.hpp>
#include <mesos/module/container_logger.hpp>
#include <mesos/slave/container_logger.hpp>
#include <mesos/slave/containerizer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/process.hpp>
#include <process/subprocess.hpp>
#include <stout/os/constants.hpp>
#include <stout/os/pipe.hpp>
#include <string>
#include <unistd.h>
#include "epoll_logger_flags.hpp"
#include "../lib/fd.hpp"

using namespace std::literals;

const std::string NAME = "mesos-epoll-logger";

struct Options final {
	std::string module_path = "/usr/sbin/mesos-epoll-logger";
	std::string log_directory = "";
};

class EpollLoggerProcess final : public process::Process<EpollLoggerProcess> {
public:
	EpollLoggerProcess(Options&& opts) : m_options(opts) {}
	~EpollLoggerProcess() noexcept {}

	auto prepare(const std::string& sandboxDirectory) -> process::Future<mesos::slave::ContainerIO> {
		auto outpipe = os::pipe();
		if (outpipe.isError()) {
			return process::Failure("epoll_logger: Failed to create pipes: "s + outpipe.error());
		}
		auto errpipe = os::pipe();
		if (errpipe.isError()) {
			os::close(outpipe->at(1));
			return process::Failure("epoll_logger: Failed to create pipes: "s + errpipe.error());
		}
		auto outfd = process::Subprocess::IO::InputFileDescriptors();
		outfd.read = outpipe->at(0);
		outfd.write = outpipe->at(1);
		auto errfd = process::Subprocess::IO::InputFileDescriptors();
		errfd.read = errpipe->at(0);
		errfd.write = errpipe->at(1);

		auto flags = EpollLoggerFlags();
		flags.stdout_fd = outpipe->at(0);
		flags.stderr_fd = errpipe->at(0);
		flags.sandbox_dir = sandboxDirectory;
		
		auto proc = process::subprocess(
			m_options.module_path,
			{ NAME },
			process::Subprocess::FD(STDIN_FILENO),
			process::Subprocess::FD(STDOUT_FILENO),
			process::Subprocess::FD(STDERR_FILENO),
			&flags,
			std::map<std::string, std::string>{ {"LIBPROCESS_IP"s, "127.0.0.1"s} },
			None(),
			{}, /* Parent hooks */
			{}, /* Child hooks */
			{ outpipe->at(0), errpipe->at(0) } /* Whitelisted fds */
		);

		// Note: Ownership of file descriptors are transferred.
		auto io = mesos::slave::ContainerIO();
		io.out = mesos::slave::ContainerIO::IO::FD(outfd.write.get());
		io.err = mesos::slave::ContainerIO::IO::FD(errfd.write.get());
		return io;
	}

private:
	Options m_options;
};

class EpollLogger final : public mesos::slave::ContainerLogger {
public:
	EpollLogger(Options&& opts) : m_process(new EpollLoggerProcess(std::move(opts))) {
		process::spawn(m_process.get());
	}

	~EpollLogger() noexcept {
		process::terminate(m_process.get());
		process::wait(m_process.get());
	}

	auto initialize() -> Try<Nothing> override { return Nothing(); }

#ifdef MESOS_1_6_API
	auto prepare(const mesos::ExecutorInfo& executorInfo, const std::string& sandboxDirectory, const Option<std::string>& user) -> process::Future<mesos::slave::ContainerIO> override {
		return process::dispatch(m_process.get(), &EpollLoggerProcess::prepare, sandboxDirectory);
	}
#else
	auto prepare(const mesos::ContainerID& containerId, const mesos::slave::ContainerConfig& containerConfig) -> process::Future<mesos::slave::ContainerIO> override {
		return process::dispatch(m_process.get(), &EpollLoggerProcess::prepare, containerConfig.directory());
	}
#endif

private:
	process::Owned<EpollLoggerProcess> m_process;
};

__attribute__((visibility("default")))
mesos::modules::Module<mesos::slave::ContainerLogger> org_personal_mesos_EpollLogger(
	MESOS_MODULE_API_VERSION,
	MESOS_VERSION,
	"Personal",
	"undisputed.seraphim@gmail.com",
	"epoll based container logger module",
	[]() -> bool { return true; },
	[](const mesos::Parameters& params) -> mesos::slave::ContainerLogger* {
		auto opts = Options();
		for (auto& param : params.parameter()) {
			if (param.key() == "module_path") {
				opts.module_path = param.value();
			}
			if (param.key() == "log_directory") {
				opts.log_directory = param.value();
			}
		}
		return new EpollLogger(std::move(opts));
	});
