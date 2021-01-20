#pragma once

#include <stout/flags.hpp>
#include <string>

class EpollLoggerFlags final : public virtual flags::FlagsBase {
public:
	EpollLoggerFlags() {
		add(&EpollLoggerFlags::stdout_fd, "stdout_fd", "Inherited file descriptor for the write end of the stdout pipe.");
		add(&EpollLoggerFlags::stderr_fd, "stderr_fd", "Inherited file descriptor for the write end of the stderr pipe.");
		add(&EpollLoggerFlags::sandbox_dir, "sandbox_dir", "Path to file where logs should be written to.");
	}

	int stdout_fd;
	int stderr_fd;
	std::string sandbox_dir;
};