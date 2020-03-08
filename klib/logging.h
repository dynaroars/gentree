//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_LOGGING_H
#define IGEN4_LOGGING_H

#include <glog/logging.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#define GLOG(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()
#define GLOG_IF(severity, condition) \
  static_cast<void>(0),             \
  !(condition) ? (void) 0 : google::LogMessageVoidify() & GLOG(severity)
#define GVLOG(verboselevel) GLOG_IF(INFO, VLOG_IS_ON(verboselevel))
#define GVLOG_IF(verboselevel, condition) \
  GLOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#undef LOG
#undef LOG_IF
#undef VLOG
#undef VLOG_IF
#undef CHECK

#define LOG(level, f, ...) GLOG(level) << fmt::format(f, ##__VA_ARGS__)
#define LOG_IF(level, cond, f, ...) GLOG_IF(level, cond) << fmt::format(f, ##__VA_ARGS__)
#define VLOG(verboselevel, f, ...) GVLOG(verboselevel) << fmt::format(f, ##__VA_ARGS__)
#define VLOG_IF(verboselevel, cond, f, ...) GVLOG_IF(verboselevel, cond) << fmt::format(f, ##__VA_ARGS__)


#define GCHECK(condition)  \
      GLOG_IF(FATAL, GOOGLE_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
             << "Check failed: " #condition " "

#define CHECK(condition) GCHECK(condition)
#define CHECKF(cond, f, ...) CHECK(cond) << fmt::format(f, ##__VA_ARGS__)

#define LOG_BLOCK(level, code) do { \
        std::stringstream log; {code;} GLOG(LEVEL) << log.rdbuf(); \
    } while(0)
#define LOG_IF_BLOCK(level, condition, code) do { \
        if (condition) { std::stringstream log; {code;} GLOG(level) << log.rdbuf(); }\
    } while(0)
#define VLOG_BLOCK(verboselevel, code) LOG_IF_BLOCK(INFO, VLOG_IS_ON(verboselevel), code)
#define VLOG_IF_BLOCK(verboselevel, condition, code) LOG_IF_BLOCK(INFO, (condition) && VLOG_IS_ON(verboselevel))

#endif //IGEN4_LOGGING_H
