#include <exception>
#include <fstream>
#include <iostream>
#include <stout/flags.hpp>
#include <stout/path.hpp>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include "epoll_logger_flags.hpp"
#include "../lib/fd.hpp"

using namespace std::literals;

void loop(const EpollLoggerFlags& flags) {
	auto stdout_file = std::ofstream(path::join(flags.sandbox_dir, "stdout"));
	auto stderr_file = std::ofstream(path::join(flags.sandbox_dir, "stderr"));
	auto epoll_fd = fd(::epoll_create1(EPOLL_CLOEXEC));
	auto event = epoll_event{};
	event.events = EPOLLIN | EPOLLERR;
	event.data.fd = flags.stdout_fd;
	if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, flags.stdout_fd, &event)) {
		int e = errno;
		//throw std::exception(("Failed to add stdout file descriptor to epoll: "s + strerror(e)).c_str());
		return;
	}
	event = epoll_event{};
	event.events = EPOLLIN | EPOLLERR;
	event.data.fd = flags.stderr_fd;
	if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, flags.stderr_fd, &event)) {
		int e = errno;
		//throw std::exception(("Failed to add stderr file descriptor to epoll: "s + strerror(e)).c_str());
		return;
	}
	char buffer[4096];
	constexpr size_t EPOLL_MAX_EVENTS = 4;
	epoll_event event_buffer[EPOLL_MAX_EVENTS];
	int count = 0;
	do {
		count = ::epoll_wait(epoll_fd, event_buffer, EPOLL_MAX_EVENTS, -1);
		if (count < 0) {
			int e = errno;
			// Error occurred on epoll wait
			return;
		}
		for (int i = 0; i < count; ++i) {
			ssize_t r = ::read(event_buffer[i].data.fd, buffer, sizeof(buffer));
			if (r <= 0) {
				return;
			}
			if (event_buffer[i].data.fd == flags.stdout_fd) {
				stdout_file << buffer << std::endl;
			}
			else {
				stderr_file << buffer << std::endl;
			}
		}
	} while (true);
}

int main(int argc, char* argv[]) {
	auto flags = EpollLoggerFlags();
	auto load = flags.load(None(), &argc, &argv);
	if (flags.help) {
		std::cout << flags.usage() << std::endl;
		return EXIT_FAILURE;
	}
	if (load.isError()) {
		std::cerr << flags.usage(load.error()) << std::endl;
		return EXIT_FAILURE;
	}
	if (::setsid() == -1) {
		EXIT(EXIT_FAILURE) << ErrnoError("Failed to put child into a new session.").message;
	}
	loop(flags);
	return 0;
}