#ifndef __SHM_H__
#define __SHM_H__

#include <stdbool.h>

typedef struct
{
    bool is_up;
    int dest_id;
}Interface;



typedef struct
{
    bool is_on;
    char virtual_ip[30];
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

typedef struct RoutingTable2
{
    int m ;
    char virtual_id[30];
    bool interfaces[64];
}Routing_table2;

Routing_table2 rt2 = {
    .m = 0
};

#endif
