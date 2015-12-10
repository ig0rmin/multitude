#include "ProcessRunner.hpp"
#include <Radiant/Platform.hpp>
#include <Radiant/Timer.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

namespace Radiant
{
  QString ProcessRunner::Result::stringStatus()
  {
    switch(status) {
    case Status::Success:
      return QString("success");
    case Status::Error:
      return QString("error");
    case Status::FailedToStart:
      return QString("failedToStart");
    case Status::Timedout:
      return QString("timedout");
    default:
      return QString("error");
    }
  }

  QString ProcessRunner::Result::toString()
  {
    return QString("Status: %1. Exit code: %2").arg(stringStatus()).arg(exitCode);
  }

  ProcessOutputHandler lineByLineHandler(const LineHandler & worker)
  {
    std::shared_ptr<int> lineStart(new int);
    *lineStart = 0;
    return [worker, lineStart](const QByteArray &output, int countNewBytes) {
      if(countNewBytes == 0) {
        // ProcessRunner is at end of output
        if(*lineStart < output.size()) {
          worker(output, *lineStart, output.size());
        }
      }
      // if have a \n in countNewBytes then call the worker
      int firstNewByte = output.size() - countNewBytes;
      for(int i = firstNewByte; i < output.size(); ++i) {
        if(output[i] == '\n') {
          worker(output, *lineStart, i + 1);
          *lineStart = i + 1;
        }
      }
    };
  }
}
