#ifndef _NET_INTERFACE_H
#define _NET_INTERFACE_H

#include <string>

namespace xop
{
    class NetInterface
    {
    public:
        static std::string getLocalIPAddress();
    };
}

#endif
