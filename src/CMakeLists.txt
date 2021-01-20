if (UNIX)

add_library(managed-fd STATIC
	"lib/fd.hpp"
	"lib/fd.cpp"
)

add_library(epoll_container_logger SHARED
	"epoll-logger/epoll_logger_flags.hpp"
	"epoll-logger/libepoll_logger.cpp"
)
add_executable(mesos-epoll-logger 
	"epoll-logger/epoll_logger_flags.hpp"
	"epoll-logger/epoll_logger.cpp"
)
target_link_libraries(epoll_container_logger PRIVATE
	managed-fd
	protobuf::libprotobuf
	Mesos::Mesos
)
target_link_libraries(mesos-epoll-logger PRIVATE
	managed-fd
	Mesos::Mesos
)

if (Mesos_VERSION VERSION_LESS "1.7.0")
	target_compile_definitions(epoll_container_logger PRIVATE MESOS_1_6_API=1)
endif()

endif()
