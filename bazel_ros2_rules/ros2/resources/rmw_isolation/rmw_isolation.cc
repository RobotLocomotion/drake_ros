#include "rmw_isolation/rmw_isolation.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include <rclcpp/rclcpp.hpp>
#include <rmw/rmw.h>

#include "tools/cpp/runfiles/runfiles.h"

#define LITERAL_STRINGIFY(x) #x
#define STRINGIFY(x) LITERAL_STRINGIFY(x)

using bazel::tools::cpp::runfiles::Runfiles;

namespace ros2 {

namespace {

// Non-copyable, non-movable wrapper to bound an object's lifetime
// to local scope. Useful for objects that do not follow RAII, such
// as heap-allocated libc objects.
template <typename ValueT, typename DeleterT>
class scoped_instance {
 public:
  scoped_instance(ValueT&& value, DeleterT&& deleter)
      : value_(value), deleter_(deleter) {}
  scoped_instance(const scoped_instance&) = delete;
  scoped_instance(scoped_instance&&) = delete;
  scoped_instance& operator=(const scoped_instance&) = delete;
  scoped_instance& operator=(scoped_instance&&) = delete;

  ~scoped_instance() { deleter_(std::move(value_)); }

  ValueT& get() { return value_; }

 private:
  ValueT value_;
  DeleterT deleter_;
};

// Helper function to enable type deduction on scoped_instance construction.
template <typename ValueT, typename DeleterT>
auto make_scoped_instance(ValueT&& value, DeleterT&& deleter) {
  return scoped_instance<ValueT, DeleterT>(std::forward<ValueT>(value),
                                           std::forward<DeleterT>(deleter));
}

}  // namespace

void isolate_rmw_by_path(const std::string& argv0, const std::string& path) {
  if (rclcpp::ok()) {
    throw std::runtime_error(
        "middleware already initialized, too late for isolation");
  }
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv0, &error));
  if (!runfiles) {
    throw std::runtime_error(error);
  }
  const std::string generate_isolated_env_rmw_location = runfiles->Rlocation(
      STRINGIFY(RMW_ISOLATION_ROOTPATH) "/generate_isolated_rmw_env");
  const std::string scratch_directory_template = path + "/XXXXXX";
  auto mutable_scratch_directory_template = make_scoped_instance<char*>(
      strdup(scratch_directory_template.c_str()), free);
  if (mkdtemp(mutable_scratch_directory_template.get()) == nullptr) {
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  std::ostringstream command_line;
  command_line << generate_isolated_env_rmw_location << " --scratch-directory "
               << mutable_scratch_directory_template.get()
               << " --rmw-implementation "
               << rmw_get_implementation_identifier() << " " << path;
  std::string command = command_line.str();
  fflush(stdout);  // Flush stdout to avoid interactions with forked process
  auto command_stream = make_scoped_instance(
      popen(command.c_str(), "r"), [&](FILE* command_stream) {
        if (command_stream != nullptr) {
          int return_code = pclose(command_stream);
          if (return_code == -1 && errno == ECHILD) {
            throw std::system_error(errno, std::system_category(),
                                    strerror(errno));
          }
          if (return_code != 0) {
            std::ostringstream error_message;
            error_message << generate_isolated_env_rmw_location
                          << " exited with " + return_code;
            throw std::runtime_error(error_message.str());
          }
        }
      });
  if (command_stream.get() == nullptr) {
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  size_t length = 0;
  auto buffer = make_scoped_instance<char*>(nullptr, free);
  while (getline(&buffer.get(), &length, command_stream.get()) != -1) {
    char* bufferp = buffer.get();  // let strsep() mutate bufferp but not buffer
    const std::string line{strsep(&bufferp, "\r\n")};  // drop trailing newline
    const size_t separator_index = line.find("=");
    const std::string name = line.substr(0, separator_index);
    if (name.empty()) {
      throw std::runtime_error("internal error: invalid environment variable");
    }
    const std::string value = separator_index != std::string::npos
                                  ? line.substr(separator_index + 1)
                                  : "";
    if (setenv(name.c_str(), value.c_str(), 1) != 0) {
      throw std::system_error(errno, std::system_category(), strerror(errno));
    }
  }
  if (!feof(command_stream.get())) {
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
}

}  // namespace ros2
