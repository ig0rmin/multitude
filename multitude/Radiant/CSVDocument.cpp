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

#include "CSVDocument.hpp"

#include "FileUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <QStringList>

namespace Radiant {

  CSVDocument::CSVDocument()
  {
  }

  CSVDocument::~CSVDocument()
  {
  }

  int CSVDocument::loadFromString(const QString &csv, const char *delimiter, bool removeQuotations)
  {
    m_rows.clear();

    if(csv.isEmpty()) {
      error("CSVParser::loadFromString # Empty contents");
      return -1;
    }

    QString delim2 = QString::fromUtf8(delimiter);

    foreach(QString line, csv.split("\n")) {
      Row r;
      foreach(QString str, line.split(delim2))
        r.push_back(str.trimmed());
      m_rows.push_back(r);
    }

    return (int) m_rows.size();
  }

  int CSVDocument::load(const char *filename, const char * delimiter, bool removeQuotations)
  {
    QString contents = Radiant::FileUtils::loadTextFile(filename);
    return loadFromString(contents, delimiter, removeQuotations);
  }

  CSVDocument::Row * CSVDocument::findRow(const QString & key, unsigned col)
  {
    for(Rows::iterator it = m_rows.begin(); it != m_rows.end(); it++) {
      Row & r = (*it);
      if(col < r.size()) {
        if(r[col] == key) {
          return & r;
        }
      }
    }

    return 0;
  }

  CSVDocument::Row * CSVDocument::row(unsigned index)
  {
    if(index >= rowCount())
      return 0;

    unsigned n = 0;
    for(Rows::iterator it = begin(); it != end(); it++, n++) {
      if(n == index)
        return & (*it);
    }

    return 0; // Should be unreachable
  }

}
