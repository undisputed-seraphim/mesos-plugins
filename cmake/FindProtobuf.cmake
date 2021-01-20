#[[.rst:
FindProtobuf
---------

Try to find the Protobuf library.
This module assumes the library was installed with pkg-config.
This module will only find the static libraries (.a).

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:
``protobuf::Protobuf``
  The Protobuf library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Protobuf_FOUND``
  True if the system has Protobuf.
``Protobuf_VERSION``
  The version of Protobuf which was found.
``Protobuf_INCLUDE_DIRS``
  The include directories needed to use Protobuf.
``Protobuf_LIBRARIES``
  The libraries needed to link against Protobuf.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Protobuf_INCLUDE_DIR``
  The directory that contains ``google/protobuf/stubs/common.hpp``.
``Protobuf_LIBRARY``
  The path to libprotobuf.a.

#]]

find_package(PkgConfig)
pkg_check_modules(_Protobuf QUIET protobuf)
find_path(Protobuf_INCLUDE_DIR
        google/protobuf/stubs/common.h
        HINTS
                ${_Protobuf_INCLUDEDIR}
                ${_Protobuf_INCLUDE_DIR})
find_library(Protobuf_LIBRARY
        NAMES
                libprotobuf.a
        HINTS
                ${_Protobuf_LIBDIR}
                ${_Protobuf_LIBRARY_DIRS})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Protobuf
        REQUIRED_VARS
                Protobuf_INCLUDE_DIR
                Protobuf_LIBRARY
        VERSION_VAR
                _Protobuf_VERSION)
if (Protobuf_FOUND AND NOT TARGET protobuf::libprotobuf)
        add_library(protobuf::libprotobuf UNKNOWN IMPORTED)
        set_target_properties(protobuf::libprotobuf PROPERTIES
                IMPORTED_LOCATION "${Protobuf_LIBRARY}"
                INTERFACE_COMPILE_OPTIONS "${_Protobuf_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${Protobuf_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES pthread)
endif()
mark_as_advanced(Protobuf_INCLUDE_DIR Protobuf_LIBRARY)
set(Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIR})
set(Protobuf_LIBRARIES "${Protobuf_LIBRARY}")
set(Protobuf_VERSION ${_Protobuf_VERSION})
