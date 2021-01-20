#[[.rst:
FindMesos
---------

Try to find the Mesos library.
This module assumes the library was installed with pkg-config.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:
``Mesos::Mesos``
  The Mesos library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Mesos_FOUND``
  True if the system has Mesos.
``Mesos_VERSION``
  The version of Mesos which was found.
``Mesos_INCLUDE_DIRS``
  The include directories needed to use Mesos.
``Mesos_LIBRARIES``
  The libraries needed to link against Mesos.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Mesos_INCLUDE_DIR``
  The directory that contains ``mesos/mesos.hpp``.
``Mesos_LIBRARY``
  The path to libmesos.so.

#]]

find_package(PkgConfig)
pkg_check_modules(_Mesos QUIET mesos)
find_path(Mesos_INCLUDE_DIR
	mesos/mesos.hpp
	HINTS
		${_Mesos_INCLUDEDIR}
		${_Mesos_INCLUDE_DIR})
find_library(Mesos_LIBRARY
	NAMES
		mesos
	HINTS
		${_Mesos_LIBDIR}
		${_Mesos_LIBRARY_DIRS})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mesos
	REQUIRED_VARS
		Mesos_INCLUDE_DIR
		Mesos_LIBRARY
	VERSION_VAR
		_Mesos_VERSION)
if (Mesos_FOUND AND NOT TARGET Mesos::Mesos)
	add_library(Mesos::Mesos UNKNOWN IMPORTED)
	set_target_properties(Mesos::Mesos PROPERTIES
		IMPORTED_LOCATION "${Mesos_LIBRARY}"
		INTERFACE_COMPILE_OPTIONS "${_Mesos_CFLAGS_OTHER}"
		INTERFACE_INCLUDE_DIRECTORIES "${Mesos_INCLUDE_DIR}"
		INTERFACE_LINK_LIBRARIES pthread)
endif()
mark_as_advanced(Mesos_INCLUDE_DIR Mesos_LIBRARY)
set(Mesos_INCLUDE_DIRS ${Mesos_INCLUDE_DIR})
set(Mesos_LIBRARIES "${Mesos_LIBRARY}")
set(Mesos_VERSION ${_Mesos_VERSION})
