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

#include "Export.hpp"
#include "Platform.hpp"

#include <stdint.h>

#include <QString>

namespace Radiant
{
  /** Small utility functions to help handle platform-specific functions. */

    /// @todo check that these actually work

  namespace PlatformUtils
  {

    /// Return absolute path to the executable that was used to launch the process.
    RADIANT_API QString getExecutablePath();

    /// Return absolute path to the user's home directory.
    RADIANT_API QString getUserHomePath();

    /// Return path to the global data directory of the given module.
    RADIANT_API QString getModuleGlobalDataPath(const char * module, bool isapplication);

    /// Return path to the user data directory of the given module.
    RADIANT_API QString getModuleUserDataPath(const char * module, bool isapplication);

    /// Determine whether file or directory is readable.
    /// @todo shouldn't this be in FileUtils?
    /// @param filename Filename to test for readability
    /// @returns true if the given file is found and readable
    RADIANT_API bool fileReadable(const char * filename);

    /// Open a dynamic library
    /// @param path Full path to plugin
    /// @returns Handle to plugin or NULL if failed
    RADIANT_API void * openPlugin(const char * path);

    /// Returns the memory usage of the process, in bytes
    /** This function is not implemented for all platforms. */
    /// @returns Size of memory usage (in bytes)
    RADIANT_API uint64_t processMemoryUsage();

    /// Setup an environment variable
    RADIANT_API void setEnv(const char * name, const char * value);

    /// Make a new TCP rule to OS firewall
    /// @param port TCP port to open
    /// @param name rule name
    RADIANT_API void openFirewallPortTCP(int port, const QString & name);

    /// Reboot the system
    /// @throw QString error message
    /// @return true on success
    RADIANT_API bool reboot();

    /// Shutdown the system immediately
    /// @return true on success
    RADIANT_API bool shutdown();

#ifdef RADIANT_WINDOWS
    /// Get path to folder used for application data that is not user specific (i.e. ProgramData)
    RADIANT_API QString windowsProgramDataPath();
#endif

  }

}

#endif
