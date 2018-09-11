// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "arrow/util/logging.h"

#ifndef _WIN32
#include <execinfo.h>
#endif

#ifdef ARROW_USE_GLOG
#include "glog/logging.h"
#endif

namespace arrow {

// This is the default implementation of arrow log,
// which is independent of any libs.
class CerrLog {
 public:
  explicit CerrLog(int severity) : severity_(severity), has_logged_(false) {}

  virtual ~CerrLog() {
    if (has_logged_) {
      std::cerr << std::endl;
    }
    if (severity_ == ARROW_FATAL) {
      PrintBackTrace();
      std::abort();
    }
  }

  std::ostream& Stream() {
    has_logged_ = true;
    return std::cerr;
  }

  template <class T>
  CerrLog& operator<<(const T& t) {
    if (severity_ != ARROW_DEBUG) {
      has_logged_ = true;
      std::cerr << t;
    }
    return *this;
  }

 protected:
  const int severity_;
  bool has_logged_;

  void PrintBackTrace() {
#if defined(_EXECINFO_H) || !defined(_WIN32)
    void* buffer[255];
    const int calls = backtrace(buffer, sizeof(buffer) / sizeof(void*));
    backtrace_symbols_fd(buffer, calls, 1);
#endif
  }
};

int ArrowLog::severity_threshold_ = ARROW_INFO;
std::unique_ptr<char, std::default_delete<char[]>> ArrowLog::app_name_;
std::string ArrowLog::working_dir_;

#ifdef ARROW_USE_GLOG

// Glog's severity map.
static int GetMappedSeverity(int severity) {
  switch (severity) {
    case ARROW_DEBUG:
      return google::GLOG_INFO;
    case ARROW_INFO:
      return google::GLOG_INFO;
    case ARROW_WARNING:
      return google::GLOG_WARNING;
    case ARROW_ERROR:
      return google::GLOG_ERROR;
    case ARROW_FATAL:
      return google::GLOG_FATAL;
    default:
      ARROW_LOG(FATAL) << "Unsupported logging level: " << severity;
      // This return won't be hit but compiler needs it.
      return google::GLOG_FATAL;
  }
}

#endif

#ifdef ARROW_USE_GLOG
typedef ::google::LogMessage LoggingProvider;
#else
class CerrLog;
typedef CerrLog LoggingProvider;
#endif

void ArrowLog::StartArrowLog(const std::string& app_name, int severity_threshold,
                             const std::string& log_dir) {
#ifdef ARROW_USE_GLOG
  severity_threshold_ = severity_threshold;
  app_name_.reset(new char[app_name.length() + 1]);
  snprintf(app_name_.get(), app_name.length() + 1, "%s", app_name.c_str());
  int mapped_severity_threshold = GetMappedSeverity(severity_threshold_);
  google::InitGoogleLogging(app_name_.get());
  google::SetStderrLogging(mapped_severity_threshold);
  // Enble log file if log_dir is not empty.
  if (!log_dir.empty()) {
    auto dir_ends_with_slash = log_dir;
    if (log_dir[log_dir.length() - 1] != '/') {
      dir_ends_with_slash += "/";
    }
    auto app_name_without_path = app_name;
    if (app_name.empty()) {
      app_name_without_path = "DefaultApp";
    } else {
      // Find the app name without the path.
      size_t pos = app_name.rfind('/');
      if (pos != app_name.npos && pos + 1 < app_name.length()) {
        app_name_without_path = app_name.substr(pos + 1);
      }
    }
    google::SetLogFilenameExtension(app_name_without_path.c_str());
    google::SetLogDestination(mapped_severity_threshold, log_dir.c_str());
  }
#endif
  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("Current working dir: %s\n", cwd);
       working_dir_ = cwd;
  }
}

void ArrowLog::ShutDownArrowLog() {
#ifdef ARROW_USE_GLOG
  google::ShutdownGoogleLogging();
#endif
}

void ArrowLog::InstallFailureSignalHandler() {
#ifdef ARROW_USE_GLOG
  google::InstallFailureSignalHandler();
#endif
}

ArrowLog::ArrowLog(const char* file_name, int line_number, int severity)
    // glog does not have DEBUG level, we can handle it here.
    : is_enabled_(severity >= severity_threshold_),
      logging_provider_(nullptr) {
#ifdef ARROW_USE_GLOG
  if (is_enabled_) {
    auto logging_provider =
        new google::LogMessage(file_name, line_number, GetMappedSeverity(severity));
    logging_provider->stream() << "(" << app_name_.get() << "): ";
    logging_provider_ = logging_provider;
    char *getcwd(char *buf, size_t size);
  }
#else
  logging_provider_ = new CerrLog(severity);
  *reinterpret_cast<LoggingProvider*>(logging_provider_) << file_name << ":" << line_number << ": ";
#endif
}

std::ostream& ArrowLog::Stream() {
#ifdef ARROW_USE_GLOG
  // Before calling this function, user should check IsEnabled.
  // When IsEnabled == false, logging_provider_ will be empty.
  return reinterpret_cast<LoggingProvider*>(logging_provider_)->stream();
#else
  return reinterpret_cast<LoggingProvider*>(logging_provider_)->Stream();
#endif
}

bool ArrowLog::IsEnabled() const { return is_enabled_; }

ArrowLog::~ArrowLog() {
  if (logging_provider_ != nullptr) {
    delete reinterpret_cast<LoggingProvider*>(logging_provider_);
  }
}

}  // namespace arrow
