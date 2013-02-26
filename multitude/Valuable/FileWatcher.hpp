/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_FILEWATCHER_HPP
#define VALUABLE_FILEWATCHER_HPP

#include "Node.hpp"

#include <QString>
#include <QStringList>

namespace Valuable
{

  /// FileWatcher provides an interface for monitoring files and directories
  /// for modifications.
  class VALUABLE_API FileWatcher : public Node
  {
  public:
    FileWatcher();
    ~FileWatcher();

    void addPath(const QString & relativePath);
    void addPaths(const QStringList & paths);

    QStringList files() const;
    QStringList directories() const;

    QStringList allWatchedDirectories() const;
    QStringList allWatchedFiles() const;

    void removePath(const QString & path);
    void removePaths(const QStringList & paths);
    void clear();

  private:
    class D;
    D * m_d;
  };
}

#endif // VALUABLE_FILEWATCHER_HPP
