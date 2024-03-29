#ifndef __SHM_H__
#define __SHM_H__

#include <stdbool.h>

typedef struct
{
    bool is_up;
    char src_virtual_ip[30];
    char dst_virtual_ip[30];
    int dest_id;
}Interface;



typedef struct
{
    bool is_on;
    char host[30];
    int port;
    int interface_size;
    Interface interface[64];
}Node;



typedef struct 
{
    int size;
    Node node[64];
}Graph;

Graph gh = {
    .size = 0
};

typedef struct RoutingTable
{
    int version;
    Graph graph;
} Routing_table;

Routing_table rt = {
    .version = 0
};


#endif
