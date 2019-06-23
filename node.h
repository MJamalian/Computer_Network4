#ifndef __NODE_H__
#define __NODE_H__

#include "shm.h"

Routing_table* Initialize_Shared_Memory(int* ShmID){
    key_t ShmKEY;
    Routing_table *ShmPTR;

    ShmKEY = ftok(".", 67);
    *ShmID = shmget(ShmKEY, sizeof(struct RoutingTable), IPC_CREAT | 0666);
    if (*ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    ShmPTR = (Routing_table *) shmat(*ShmID, NULL, 0);
    if ((Routing_table *) ShmPTR == -1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
    }
    return ShmPTR;
}


Graph AddNewNode(char* fileName, Graph graph, int* nodeNum){
    int port;
    int ret;
    char lhost[HOST_MAX_LENGTH];
    char rhost[HOST_MAX_LENGTH], lv[INET_ADDRSTRLEN], rv[INET_ADDRSTRLEN];
    FILE *f;
    f = fopen(fileName, "r");
    fscanf(f, "%s %d", lhost, &port);
    bool check = false;
    Node node;
    while (!feof(f)) {
		ret = fscanf(f, "%s %d %s %s", rhost, &port, lv, rv);
        if (ret == EOF) {
			ret = 0;
			break;
		}
        if (!check)
        {
            for (int i = 0; i < graph.size; i++)
            {   
                if (strcmp(graph.node[i].virtual_ip, lv) == 0){
                    node = graph.node[i];
                    *nodeNum = i;
                    node.is_on = true;
                    check = true;
                    break;
                }
            }
            if (!check)
            {
                *nodeNum = graph.size;
                node.is_on = true;
                strncpy(node.virtual_ip, lv, 30);
                node.interface_size = 0;
                graph.node[*nodeNum] = node;
                graph.size++;
            }
            check = true;
        }
        
        
        int dest_num;
        bool check2 = false;
        for (int i = 0; i < graph.size; i++)
        {
            if (strcmp(graph.node[i].virtual_ip,rv) == 0)
            {
                dest_num = i;
                check2 = true;
                break;
            }
        }
        if(!check2){
            Node node2;
            node2.is_on = false;
            node2.interface_size = 0;
            strncpy(node2.virtual_ip, rv, 30);
            // node2.virtual_ip = rv;
            graph.node[graph.size] = node2;
            graph.size = graph.size + 1;
            dest_num = graph.size - 1;
        }
        Interface interface;
        interface.dest_id = dest_num;
        interface.is_up = true;
        node.interface[node.interface_size] = interface;
        node.interface_size = node.interface_size + 1;
    }
    graph.node[*nodeNum] = node;
    return graph;
}

bool is_empty(Graph graph){
    for (int i = 0; i < graph.size; i++)
    {
        if (graph.node[i].is_on)
        {
            return false;
        }  
    }
    return true;
    
}



#endif
