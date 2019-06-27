#ifndef __NETINET_IP_H
#define __NETINET_IP_H 1

#include <features.h>
#include <sys/types.h>
#include <netinet/in.h>

__BEGIN_DECLS

typedef struct IP{
    int protocol;
    char source_ip[30];
    char dest_ip[30];
    char payload[1000];
}ip_packet;

#endif /* netinet/ip.h */
