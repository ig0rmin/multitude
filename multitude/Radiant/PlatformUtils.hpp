/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef RADIANT_PLATFORM_UTILS_HPP
#define RADIANT_PLATFORM_UTILS_HPP

#include <Radiant/Export.hpp>

#include <cstdint>

#include <QString>

namespace Radiant
{
  /** Small utility functions to help handle platform-specific functions. */

  namespace PlatformUtils
  {

    /// Return absolute path to the executable that was used to launch the process.
    RADIANT_API QString getExecutablePath();

    /// Returns the current process identifier
    RADIANT_API int getProcessId();

    /// Return absolute path to the user's home directory.
    RADIANT_API QString getUserHomePath();

    /// Return path to the global data directory of the given module.
    RADIANT_API QString getModuleGlobalDataPath(const char * module, bool isapplication);

    /// Return path to the user data directory of the given module.
    RADIANT_API QString getModuleUserDataPath(const char * module, bool isapplication);

    /// Open a dynamic library
    /// @param path Full path to plugin
    /// @returns Handle to plugin or NULL if failed
    RADIANT_API void * openPlugin(const char * path);

    /// Returns the memory usage of the process, in bytes
    /** This function is not implemented for all platforms. */
    /// @returns Size of memory usage (in bytes)
    RADIANT_API uint64_t processMemoryUsage();

    /// This function returns the path to a library the running process is
    /// linked against.
    /// @param libraryName (part of) linked library name to search for
    /// @return absolute path to the library file
    RADIANT_API QString getLibraryPath(const QString& libraryName);
  }

}

#endif
