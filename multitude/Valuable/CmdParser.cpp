/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "CmdParser.hpp"

#include "Valuable/DOMDocument.hpp"
#include "Valuable/DOMElement.hpp"
#include "Valuable/Node.hpp"
#include "Valuable/AttributeBool.hpp"

#include <QStringList>

namespace Valuable
{
  QStringList CmdParser::parse(int & argc, char * argv[],
                               Valuable::Node & opts)
  {
    CmdParser parser;
    return parser.parseAndStore(argc, argv, opts);
  }

  QStringList CmdParser::parse(const QStringList & argv,
                               Valuable::Node & opts)
  {
    CmdParser parser;
    return parser.parseAndStore(argv, opts);
  }

  bool CmdParser::isParsed(const QString & name)
  {
    return m_parsedArgs.contains(name);
  }

  QStringList CmdParser::parseAndStore(int & argc, char * argv[],
                                       Valuable::Node & opts)
  {
    QStringList tmp;
    for(int i = 1; i < argc; ++i)
      tmp << argv[i];

    QStringList out = parseAndStore(tmp, opts);

    if(out.size() > 0) {
      int argv_out = 1;
      for(int i = 1, j = 0; i < argc; ++i) {
        if(j < out.size() && argv[i] == out[j]) {
          argv[argv_out++] = argv[i];
          ++j;
        }
      }
      argc = argv_out;
    }
    return out;
  }

  QStringList CmdParser::parseAndStore(const QStringList & argv,
                                       Valuable::Node & opts)
  {
    QStringList list;

    std::shared_ptr<Valuable::DOMDocument> tmpDoc(Valuable::DOMDocument::createDocument());

    int argc = argv.size();
    for(int i = 0; i < argc; i++) {
      const QString & arg = argv[i];
      QString name;

      if(arg.length() == 2 && arg[0] == '-') {
        name = arg[1];
      } else if(arg.length() > 2 && arg.startsWith("--")) {
        name = arg.mid(2);
      } else {
        list.push_back(arg);
        continue;
      }

      Valuable::Attribute * obj = opts.getValue(name);
      if(obj) {
        Valuable::AttributeBool * b = dynamic_cast<Valuable::AttributeBool*>(obj);
        if(b) {
          *b = true;
          m_parsedArgs.insert(name);
        } else if (i < argc - 1) {
          Valuable::DOMElement e = tmpDoc->createElement("tmp");
          e.setTextContent(argv[++i]);
          obj->deserializeXML(e);
          m_parsedArgs.insert(name);
        } else {
          list.push_back(arg);
          Radiant::error("Command line parameter %s is missing an argument", name.toUtf8().data());
        }
      } else {
        if(name.length() > 3 && name.startsWith("no-")) {
          Valuable::AttributeBool * b = dynamic_cast<Valuable::AttributeBool*>(
              opts.getValue(name.mid(3)));
          if(b) {
            *b = false;
            m_parsedArgs.insert(name);
            continue;
          }
        }
        list.push_back(arg);
        //Radiant::error("Unknown command line parameter %s", name.c_str());
      }
    }
    return list;
  }
}
