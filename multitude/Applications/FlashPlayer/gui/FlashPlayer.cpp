#include "FlashPlayer.hpp"
#include "Options.hpp"

#include <QApplication>
#include <QDir>
#include <QDomDocument>

#include <limits>
#include <iostream>

#include <X11/extensions/Xinerama.h>

namespace FlashPlayer
{
  QString Screen::rectToId(const QRect & rect)
  {
    if(rect.isValid())
      return QString("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
    else
      return "";
  }
  QRect Screen::idToRect(const QString & id)
  {
    QRegExp r("(\\d+)x(\\d+)\\+(\\d+)\\+(\\d+)");
    if(r.exactMatch(id)) {
      return QRect(r.cap(3).toInt(), r.cap(4).toInt(), r.cap(1).toInt(), r.cap(2).toInt());
    } else {
      return QRect();
    }
  }

  bool Screens::update()
  {
    Display * display = XOpenDisplay(NULL);
    if(!display)
      return false;

    if(XineramaIsActive(display)) {
      int nscreens = 0;
      XineramaScreenInfo * screens = XineramaQueryScreens(display, &nscreens);
      for(int i = 0; i < nscreens; ++i) {
        Screen s = {screens[i].screen_number, screens[i].x_org, screens[i].y_org,
                    screens[i].width, screens[i].height};
        m_view |= s;
        m_screens << s;
      }
    }

    if(m_view.isNull()) {
      int nscreens = XScreenCount(display);
      for(int i = 0; i < nscreens; ++i) {
        Screen s = {i, m_view.isNull() ? 0 : m_view.width(), 0,
                    XDisplayWidth(display, i), XDisplayHeight(display, i)};
        m_view |= s;
        m_screens << s;
      }
    }

    XCloseDisplay(display);
    return m_view.isValid();
  }

  QString Screens::id() const
  {
    return Screen::rectToId(m_view);
  }

  struct ConfigLine
  {
    bool automatic;
    QString view;
    ConfigLine() : automatic(false) {}
  };

  class Config
  {
  public:
    Config(QString dir, QString file) : m_document("flash"), m_filename(dir+"/"+file)
    {
      QDir qdir;
      qdir.mkpath(dir);
      QFile qfile(m_filename);
      if(qfile.open(QIODevice::ReadOnly)) {
        m_document.setContent(&qfile);
      }
    }

    ConfigLine operator[](QString id) const
    {
      QDomElement root = m_document.documentElement();

      QDomNode n = root.firstChild();
      while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.tagName() == "config" && e.attribute("id") == id) {
          ConfigLine line;
          QString a = e.attribute("automatic");
          line.automatic = a == "yes" || a == "1" || a == "true" || a == "t";
          line.view = e.text();
          return line;
        }
        n = n.nextSibling();
      }
      return ConfigLine();
    }

    void set(QString id, ConfigLine line)
    {
      QDomElement root = m_document.documentElement();

      QDomNode n = root.firstChild();
      while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.tagName() == "config" && e.attribute("match") == id) {
          e.setAttribute("automatic", line.automatic ? "yes" : "no");
          while(e.hasChildNodes())
            e.removeChild(e.childNodes().at(0));
          e.appendChild(m_document.createTextNode(line.view));
          return;
        }
        n = n.nextSibling();
      }
      QDomElement e = m_document.createElement("config");
      e.setAttribute("automatic", line.automatic ? "yes" : "no");
      e.setAttribute("match", id);
      e.appendChild(m_document.createTextNode(line.view));
      root.firstChild().appendChild(e);
    }

    void save()
    {
      QFile qfile(m_filename);
      if(qfile.open(QIODevice::WriteOnly)) {
        qfile.write(m_document.toByteArray());
      }
    }

  private:
    QDomDocument m_document;
    QString m_filename;
  };
}

int main(int argc, char * argv[])
{
  const char * binary = "nspluginplayer-mt";
  std::vector<std::string> args;
  args.push_back(binary);

  for(int i = 1; i < argc; ++i) {
    if(argv[i] == std::string("--help") || argv[i] == std::string("-h")) {
      std::cout << "Usage: " << argv[0] << " <options> <filename or URI> <attributes>\n"
      "\n"
      "Options:\n"
      "  --verbose               enable verbose mode\n"
      "  --fullscreen            start in fullscreen mode\n"
      "  --view=<WxH+X+Y>        window size & position\n"
      "                          (example --view 400x300+100+0)\n"
      "\n"
      "Common attributes include:\n"
      "  embed                   use NP_EMBED mode\n"
      "  full                    use NP_FULL mode (default)\n"
      "  type=MIME-TYPE          MIME type of the object\n"
      "  width=WIDTH             width (in pixels)\n"
      "  height=HEIGHT           height (in pixels)\n"
      "\n"
      "Other attributes will be passed down to the plugin (e.g. flashvars)" << std::endl;
      return 0;
    }
  }

  {
    QApplication app(argc, argv);
    FlashPlayer::Screens screens;
    screens.update();
    QString id = screens.id();
    FlashPlayer::Config config(QDir::homePath() + "/.MultiTouch", "flash.xml");
    FlashPlayer::ConfigLine line = config[id];
    if(line.view.isEmpty())
      line = config["default"];

    if(!line.automatic) {
      // open gui, get result, update config
      FlashPlayer::Options options(screens, FlashPlayer::Screen::idToRect(id));
      options.show();
      app.exec();

      config.set(id, line);
      config.save();
    }

    if(!line.view.isEmpty())
      args.push_back(("--view="+line.view).toStdString());

    bool got_file = false;
    for(int i = 1; i < argc; ++i) {
      if(!got_file && argv[i][0] != 0 && argv[i][0] != '-') {
        got_file = true;
        args.push_back(std::string("src=")+argv[i]);
      } else {
        args.push_back(argv[i]);
      }
    }
  }

  char * argv2[args.size()+1];
  for(size_t i = 0; i < args.size(); ++i)
    argv2[i] = strdup(args[i].c_str());
  argv2[args.size()] = 0;

  args.clear();
  return execv(binary, argv2);
}
