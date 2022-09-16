#pragma once

#include <string>
#include <vector>

/// Call exec() on another executable with arguments and modified environment
/// variables.
/**
 * This is meant to be called in main() of a generated shim executable.
 *
 * \param argc count of arguments in argv.
 * \param argv process arguments.
 * \param executable_path path to an executable to execute.
 * \param names environment variables to be modified.
 * \param actions actions to be performed on each named environment variable.
 */
int do_dload_shim(
  int argc,
  const char ** argv,
  const char * executable_path,
  std::vector<const char *> names,
  std::vector<std::vector<const char *>> actions);
