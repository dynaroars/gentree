//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_LOGGING_H
#define IGEN4_LOGGING_H

#include <glog/logging.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#define FLOG(level, f, ...) LOG(level) << fmt::format(f, ##__VA_ARGS__)
#define FLOG_IF(level, cond, f, ...) LOG_IF(level, cond) << fmt::format(f, ##__VA_ARGS__)
#define FVLOG(level, f, ...) VLOG(level) << fmt::format(f, ##__VA_ARGS__)
#define FVLOG_IF(level, cond, f, ...) VLOG_IF(level, cond) << fmt::format(f, ##__VA_ARGS__)
#define FCHECK(cond, f, ...) CHECK(cond) << fmt::format(f, ##__VA_ARGS__)
#define FDCHECK(cond, f, ...) DCHECK(cond) << fmt::format(f, ##__VA_ARGS__)

#endif //IGEN4_LOGGING_H
