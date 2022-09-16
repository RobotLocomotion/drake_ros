#pragma once

#include <string>

namespace ros2 {

/// Isolates rmw implementation network traffic.
/**
 * This function relies on the `generate_isolated_rmw_env` CLI to
 * populate the calling process environment and achieve network
 * isolation.
 *
 * \param argv0 program name to help locate `generate_isolated_rmw_env`
 * \param path unique path to use as a basis for isolation.
 * \throws std::runtime_error if called after rmw initialization.
 * \throws std::runtime_error if `isolated_rmw_env` exits abnormally.
 * \throws std::system_error if a system call fails.
 */
void isolate_rmw_by_path(const std::string& argv0, const std::string& path);

}  // namespace ros2
