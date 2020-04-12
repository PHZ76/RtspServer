// PHZ
// 2018-5-15

#include "NetInterface.h"
#include "Socket.h"

using namespace xop;

std::string NetInterface::GetLocalIPAddress()
{
#if defined(__linux) || defined(__linux__) 
    SOCKET sockfd = 0;
    char buf[512] = { 0 };
    struct ifconf ifconf;
    struct ifreq  *ifreq;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET)
    {
        close(sockfd);
        return "0.0.0.0";
    }

    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;
    if (ioctl(sockfd, SIOCGIFCONF, &ifconf) < 0)
    {
        close(sockfd);
        return "0.0.0.0";
    }

    close(sockfd);

    ifreq = (struct ifreq*)ifconf.ifc_buf;
    for (int i = (ifconf.ifc_len / sizeof(struct ifreq)); i>0; i--)
    {
        if (ifreq->ifr_flags == AF_INET)
        {
            if (strcmp(ifreq->ifr_name, "lo") != 0)
            {
                return inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
            }
            ifreq++;
        }
    }
    return "0.0.0.0";
#elif defined(WIN32) || defined(_WIN32)
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long size = sizeof(IP_ADAPTER_INFO);

    int ret = GetAdaptersInfo(pIpAdapterInfo, &size);
    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[size];
        ret = GetAdaptersInfo(pIpAdapterInfo, &size);
    }

    if (ret != ERROR_SUCCESS)
    {
        delete pIpAdapterInfo;
        return "0.0.0.0";
    }

    while (pIpAdapterInfo)
    {
        IP_ADDR_STRING *pIpAddrString = &(pIpAdapterInfo->IpAddressList);
        while(pIpAddrString)
        {
            if (strcmp(pIpAddrString->IpAddress.String, "127.0.0.1")!=0
                && strcmp(pIpAddrString->IpAddress.String, "0.0.0.0")!=0)
            {
                // pIpAddrString->IpMask.String 
                //pIpAdapterInfo->GatewayList.IpAddress.String
                std::string ip(pIpAddrString->IpAddress.String);
                //delete pIpAdapterInfo;
                return ip;
            }		
            pIpAddrString = pIpAddrString->Next;
        } while (pIpAddrString);
        pIpAdapterInfo = pIpAdapterInfo->Next;
    }

    delete pIpAdapterInfo;
    return "0.0.0.0";
#else
    return "0.0.0.0";
#endif
}


