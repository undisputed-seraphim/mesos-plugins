#pragma once
#include <array>

struct fd final {
	fd() : m_fd(-1) {}
	explicit fd(int fd_) : m_fd(fd_) {}
	fd(fd&) = delete;
	fd(fd&& other) noexcept {
		m_fd = other.m_fd;
		other.m_fd = -1;
	}
	~fd() noexcept;

	operator int() const noexcept {
		return m_fd;
	}

	int release() noexcept {
		int fd = m_fd;
		m_fd = -1;
		return fd;
	}

	bool valid() const noexcept;

	int swap(int) noexcept;

private:
	int m_fd;
};

bool create_pipe(fd& read, fd& write) noexcept;