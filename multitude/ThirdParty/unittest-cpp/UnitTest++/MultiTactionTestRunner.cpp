#include "MultiTactionTestRunner.h"
#include "UnitTest++.h"
#include "XmlTestReporter.h"
#include "TestReporterStdout.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QDomDocument>
#include <QFile>
#include <QMap>

#include <fstream>
#include <functional>
#include <cstdio>
#include <mutex>
#include <memory>

namespace UnitTest
{
  namespace
  {
    static std::mutex s_argumentsMutex;
    static QStringList s_arguments;

    int mergeXml(QDomDocument & doc, const QString & filename)
    {
      QFile file(filename);
      if (!file.open(QFile::ReadOnly)) {
        fprintf(stderr, "mergeXml # Failed to open %s: %s\n",
                filename.toUtf8().data(),
                file.errorString().toUtf8().data());
        return 1;
      }

      if (doc.isNull()) {
        doc.setContent(&file);
      } else {
        QDomDocument importDoc;
        importDoc.setContent(&file);
        QDomElement importRoot = importDoc.documentElement();
        QDomElement root = doc.documentElement();

        // copy all nodes
        QDomNode n = importRoot.firstChild();
        while (!n.isNull()) {
          root.appendChild(doc.importNode(n, true));
          n = n.nextSibling();
        }

        // copy all attributes, sum numeric attributes (failures, tests, failedtests, time)
        QDomNamedNodeMap importAttrs = importRoot.attributes();
        for (int i = 0; i < importAttrs.count(); ++i) {
          QDomAttr attr = importAttrs.item(i).toAttr();
          if (attr.isNull())
            continue;
          if (root.hasAttribute(attr.name())) {
            bool ok;
            double a = root.attribute(attr.name()).toDouble(&ok);
            if (!ok)
              continue;
            double b = attr.value().toDouble(&ok);
            if (!ok)
              continue;
            root.setAttribute(attr.name(), QString::number(a+b));
          } else {
            root.setAttribute(attr.name(), attr.value());
          }
        }
      }
      return 0;
    }

    int listTests()
    {
      auto test = UnitTest::Test::GetTestList().GetHead();
      int i = 0;
      while (test) {
        printf("%d\t%s/%s\n", ++i, test->m_details.suiteName, test->m_details.testName);
        test = test->m_nextTest;
      }
      return 0;
    }

    int runOneTest(int index, int count, const UnitTest::Test* const test, QString xmlOutput,
                   const char *procName)
    {
      QProcess process;
      process.setProcessChannelMode(QProcess::ForwardedChannels);
      QStringList newArgs;
      QString singleArg = test->m_details.suiteName;
      singleArg += "/";
      singleArg += test->m_details.testName;
      newArgs << "--single" << singleArg << xmlOutput;
      if (QFile::exists(xmlOutput))
        QFile::remove(xmlOutput);

      printf("%2d/%2d: Running test %s/%s\n",
             index, count, test->m_details.suiteName, test->m_details.testName);
      process.start(procName, newArgs, QProcess::ReadOnly);
      process.waitForStarted();
      process.waitForFinished(15*60*1000);

      int procExitCode = process.exitCode();

      if(procExitCode) {
        printf("Test %s failed. See %s for details.\n",
               test->m_details.testName, xmlOutput.toUtf8().data());
        return procExitCode;
      } else if (process.exitStatus() == QProcess::CrashExit) {
        printf("Test %s crashed. See %s for details.\n",
               test->m_details.testName, xmlOutput.toUtf8().data());
        return 1;
      }

      return procExitCode;
    }

    std::vector<const UnitTest::Test*> filteredTests(QString match)
    {
      std::vector<const UnitTest::Test*> toRun;
      QRegExp matchRegex(match);
      auto test = UnitTest::Test::GetTestList().GetHead();
      while (test) {
        QString testName = test->m_details.testName;
        QString suiteName = test->m_details.suiteName;
        QString matchCandidate = suiteName + "/" + testName;
        if(matchRegex.isEmpty() || matchCandidate.contains(matchRegex)) {
          toRun.push_back(test);
        }
        test = test->m_nextTest;
      }
      return toRun;
    }

    void printTestReport(const QDomDocument & doc)
    {
      auto root = doc.documentElement();
      int failedTests = root.attribute("failedtests").toInt();
      int tests = root.attribute("tests").toInt();
      int failures = root.attribute("failures").toInt();
      float time = root.attribute("time").toFloat();

      int secs = time;
      int mins = secs / 60;
      secs = secs % 60;
      printf("Ran %d test in %d min %d s\n", tests, mins, secs);
      if (failedTests == 0 && failures == 0) {
        printf("No errors\n");
      } else {
        printf("FAILED - %d failed tests, %d errors\n", failedTests, failures);
        auto testElements = root.elementsByTagName("test");
        for (int i = 0; i < testElements.size(); ++i) {
          QDomElement e = testElements.item(i).toElement();
          auto failureElements = e.elementsByTagName("failure");
          if (!failureElements.isEmpty()) {
            printf("\n%s/%s [%.3f s]: %d %s:\n", e.attribute("suite").toUtf8().data(),
                   e.attribute("name").toUtf8().data(), e.attribute("time").toFloat(),
                   failureElements.size(), failureElements.size() == 1 ? "error" : "errors");
            QMap<QString, int> count;
            QStringList messages;
            for (int j = 0; j < failureElements.size(); ++j) {
              auto msg = failureElements.item(j).toElement().attribute("message");
              if (count[msg]++ == 0) {
                messages << msg;
              }
            }
            for (auto msg: messages) {
              int c = count[msg];
              if (c > 1) {
                printf("  %s [%d times]\n", msg.toUtf8().data(), c);
              } else {
                printf("  %s\n", msg.toUtf8().data());
              }
            }
          }
        }
      }
    }

    int runTests(QString match, QString xmlOutput, const char *procName)
    {
      std::vector<const UnitTest::Test*> toRun = filteredTests(match);
      int count = toRun.size();

      if (xmlOutput.isEmpty())
        xmlOutput = "TestTemp.xml";

      QDomDocument dom;
      int index = 0;
      int exitCode = 0;
      for(const UnitTest::Test * const test : toRun) {
        ++index;
        // run the test in a subprocess. Creating a MultiWidgets::Application after one
        // has been destroyed does not work properly.
        int procExitCode = runOneTest(index, count, test, xmlOutput, procName);

        exitCode = std::max(exitCode, procExitCode);
        exitCode = std::max(exitCode, mergeXml(dom, xmlOutput));
      }
      if (!xmlOutput.isEmpty()) {
        QFile output(xmlOutput);
        if (!output.open(QFile::WriteOnly)) {
          fprintf(stderr, "Failed to open %s: %s\n",
                  xmlOutput.toUtf8().data(),
                  output.errorString().toUtf8().data());
          return 1;
        }
        output.write(dom.toString().toUtf8());
      }
      if(index == 0) {
        fprintf(stderr, "Failed to find tests with name or suite matching %s\n",
                match.toUtf8().data());
        return 1;
      }

      printTestReport(dom);

      return exitCode;
    }

    int runSingleTest(QString testName, QString testSuite, QString xmlOutput)
    {
      std::unique_ptr<std::ofstream> xmlStream;
      std::unique_ptr<UnitTest::TestReporter> reporter;
      if(!xmlOutput.isEmpty()) {
        xmlStream.reset(new std::ofstream(xmlOutput.toUtf8().data()));
        reporter.reset(new UnitTest::XmlTestReporter(*xmlStream));
      } else {
        reporter.reset(new UnitTest::TestReporterStdout());
      }
      UnitTest::TestRunner runner(*reporter);
      int foundCount = 0;
      auto predicate = [&foundCount, testName, testSuite](const UnitTest::Test *const test) {
        bool found = testName == test->m_details.testName
            && (testSuite.isEmpty() || testSuite == test->m_details.suiteName);
        if(found) {
          foundCount++;
        }
        return found;
      };
      int errorCode = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
      if(foundCount == 0) {
        fprintf(stderr, "Failed to find test '%s' in suite '%s'\n",
                testName.toUtf8().data(),
                testSuite.toUtf8().data());
        return 1;
      }
      if(foundCount > 1) {
        fprintf(stderr, "Found more than one test with name '%s' in suite '%s'\n",
                testName.toUtf8().data(),
                testSuite.toUtf8().data());
        return 1;
      }
      return errorCode;
    }
  }  // unnamed namespace

  int runTests(int argc, char ** argv)
  {
    QCoreApplication app(argc, argv);

    /// Avoid number separator mess
    setlocale(LC_NUMERIC, "C");

    QStringList cmdLineArgs;
    for(int i = 0; i < argc; ++i) {
      cmdLineArgs << QString(argv[i]);
    }
    {
      std::lock_guard<std::mutex> guard(s_argumentsMutex);
      s_arguments = cmdLineArgs;
    }

    QCommandLineParser parser;
    QCommandLineOption singleOption("single",
                                    "Run a single test without creating a subprocess.",
                                    "TEST_NAME");
    QCommandLineOption listOption("list", "List all available tests.");
    QCommandLineOption matchOption("match",
                                   "Run only the tests that match the given regex.",
                                   "REGEX");
    parser.addOptions({singleOption, listOption, matchOption});
    parser.addPositionalArgument("xmlFile", "XML file for the test status output");
    parser.addHelpOption();
    parser.process(cmdLineArgs);

    QStringList positional = parser.positionalArguments();
    if (positional.size() > 1) {
      positional.removeFirst();
      fprintf(stderr, "Found extra command line arguments: %s\n",
              positional.join(" ").toUtf8().data());
      return 1;
    }
    QString xmlOutput = positional.empty() ? QString() : positional.front();

    if (parser.isSet("list")) {
      return listTests();
    }

    QString single = parser.value("single");
    if(!single.isEmpty()) {
      QStringList parts = single.split("/");
      QString name, suite;
      if(parts.size() == 1) {
        name = parts[0];
      }
      if(parts.size() == 2) {
        suite = parts[0];
        name = parts[1];
      }
      if(parts.size() == 0 || parts.size() > 2) {
        fprintf(stderr, "Invalid argument to --single. Expecting suiteName/testName or just testName\n");
        return 1;
      }
      return runSingleTest(name, suite, xmlOutput);
    } else {
      runTests(parser.value("match"), xmlOutput, argv[0]);
    }
    return 0;
  }

  QStringList getCommandLineArgs()
  {
    std::lock_guard<std::mutex> guard(s_argumentsMutex);
    return s_arguments;
  }
}
