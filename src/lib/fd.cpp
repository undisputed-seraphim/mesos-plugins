#include "fd.hpp"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include "fd.hpp"

fd::~fd() noexcept {
	if (m_fd > -1) {
		::close(m_fd);
		m_fd = -1;
	}
}

bool fd::valid() const noexcept {
	return (m_fd > -1) && (::fcntl(m_fd, F_GETFD) != -1 || errno != EBADF);
}

int fd::swap(int fd) noexcept {
	return std::exchange(m_fd, fd);
}

bool create_pipe(fd& read, fd& write) noexcept {
	int fds[2];
	int ok = pipe2(fds, O_CLOEXEC);
	read.swap(fds[0]);
	write.swap(fds[1]);
	return ok == 0;
}