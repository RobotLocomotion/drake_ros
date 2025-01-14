#include "network_isolation.h"

#include <iostream>

#include <ifaddrs.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/types.h>

namespace network_isolation {

void error(const char * message)
{
    std::cerr << "create_linux_namesapces: "
        << message << ":" << strerror(errno) << "\n";
}

bool create_linux_namespaces()
{
    int result = unshare(CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWIPC);

    if (result != 0) {
        error("failed to call unshare");
        return false;
    }

    // Assert there is exactly one network interface
    struct ifaddrs *ifaddr;

    if (-1 == getifaddrs(&ifaddr)) {
        error("could not get network interfaces");
        return false;
    }
    if (nullptr == ifaddr) {
        error("there are no network interfaces");
        return false;
    }
    if (nullptr != ifaddr->ifa_next) {
        error("there are multiple network interfaces");
        return false;
    }

    // Need a socket to do ioctl stuff on
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if( fd < 0 ){
        error("could not open a socket");
        freeifaddrs(ifaddr);
        return false;
    }

    struct ifreq ioctl_request;

    // Check what flags are set on the interface
    strncpy(ioctl_request.ifr_name, ifaddr->ifa_name, IFNAMSIZ);
    int err = ioctl(fd, SIOCGIFFLAGS, &ioctl_request);
    if (0 != err) {
        freeifaddrs(ifaddr);
        error("failed to get interface flags");
        return false;
    }

    // Expecting a loopback interface.
    if (!(ioctl_request.ifr_flags & IFF_LOOPBACK)) {
        error("the only interface is not a loopback interface");
        freeifaddrs(ifaddr);
        return false;
    }

    // Enable multicast
    ioctl_request.ifr_flags |= IFF_MULTICAST;
    // Bring up interface
    ioctl_request.ifr_flags |= IFF_UP;

    err = ioctl(fd, SIOCSIFFLAGS, &ioctl_request);
    if (0 != err) {
        error("failed to set interface flags");
        freeifaddrs(ifaddr);
        return false;
    }

    // For programs that use both LCM and ROS, we need an LCM route ala
    //  sudo route add -net 224.0.0.0 netmask 240.0.0.0 dev lo
    struct rtentry route = {};
    auto* dest = reinterpret_cast<struct sockaddr_in*>(&route.rt_dst);
    dest->sin_family = AF_INET;
    dest->sin_addr.s_addr = inet_addr("224.0.0.0");
    auto* mask = reinterpret_cast<struct sockaddr_in*>(&route.rt_genmask);
    mask->sin_family = AF_INET;
    mask->sin_addr.s_addr = inet_addr("240.0.0.0");
    std::string device{"lo"};
    route.rt_dev = device.data();
    route.rt_flags = RTF_UP;
    err = ioctl(fd, SIOCADDRT, &route);
    if (0 != err) {
        error("failed to set route");
        freeifaddrs(ifaddr);
        return false;
    }

    freeifaddrs(ifaddr);
    return true;
}
}  // namespace network_isolation
