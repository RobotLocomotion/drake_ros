#pragma once

namespace network_isolation {
/// Creates linux namespaces suitable for isolating ROS 2 traffic.
///
/// The new namespaces are:
/// * A new user namespace to avoid needing CAP_SYS_ADMIN to create
///   network and IPC namespaces
/// * A new network namespace to prevent cross-talk via the network
/// * A new IPC namespaces to prevent cross-talk via shared memory
///
/// It also configures network namespace to enable ROS 2 traffic.
/// At the end of a successful call the current process will be in
/// the created namespaces.
/// Depending on what part of the process fails, an unsuccessful
/// call may also leave the current process in new namespaces.
/// There is no way to undo this.
///
/// \return true iff the namespaces were created successfully.
bool create_linux_namespaces();

}  // namespace network_isolation
