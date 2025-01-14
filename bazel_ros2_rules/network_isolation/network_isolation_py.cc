#include <pybind11/pybind11.h>

#include "network_isolation/network_isolation.h"

namespace py = pybind11;

PYBIND11_MODULE(network_isolation_py, m)
{
  m.def("create_linux_namespaces", &network_isolation::create_linux_namespaces, R"pbdoc(
        Creates linux namespaces suitable for isolating ROS 2 traffic.
    )pbdoc");
}
