#ifndef __NODE_H__
#define __NODE_H__

#include "shm.h"

extern Graph graph;
extern int nodeNum;
extern int version;
extern int version_fd;
extern int distance[64];
extern int pred[64];

Routing_table* Initialize_Shared_Memory(int* ShmID){
    key_t ShmKEY;
    Routing_table *ShmPTR;

    ShmKEY = ftok(".", 69);
    *ShmID = shmget(ShmKEY, sizeof(struct RoutingTable), IPC_CREAT | 0666);
    if (*ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    ShmPTR = (Routing_table *) shmat(*ShmID, NULL, 0);
    if ((Routing_table *) ShmPTR == (Routing_table *)-1) {
          printf("*** shmat error (server) ***\n");
          exit(1);
    }
    return ShmPTR;
}


Graph AddNewNode(char* fileName){
    int port, dest_port;
    int ret;
    char lhost[HOST_MAX_LENGTH];
    char rhost[HOST_MAX_LENGTH], lv[INET_ADDRSTRLEN], rv[INET_ADDRSTRLEN];
    FILE *f;
    f = fopen(fileName, "r");
    fscanf(f, "%s %d", lhost, &port);
    bool check = false;
    Node node;
    while (!feof(f)) {
		ret = fscanf(f, "%s %d %s %s", rhost, &dest_port, lv, rv);
        if (ret == EOF) {
			ret = 0;
			break;
		}
        if (!check)
        {
            for (int i = 0; i < graph.size; i++)
            {   
                if (graph.node[i].port == port){
                    node = graph.node[i];
                    nodeNum = i;
                    node.is_on = true;
                    check = true;
                    break;
                }
            }
            if (!check)
            {
                nodeNum = graph.size;
                node.is_on = true;
                node.port = port;
                node.interface_size = 0;
                graph.node[nodeNum] = node;
                graph.size++;
            }
            check = true;
        }
        
        
        int dest_num;
        bool check2 = false;
        for (int i = 0; i < graph.size; i++)
        {
            if (graph.node[i].port == dest_port)
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
            node2.port = dest_port;
            //strncpy(node2.virtual_ip, rv, 30);
            // node2.virtual_ip = rv;
            graph.node[graph.size] = node2;
            graph.size = graph.size + 1;
            dest_num = graph.size - 1;
        }
        Interface interface;
        interface.dest_id = dest_num;
        interface.is_up = true;
        strncpy(interface.src_virtual_ip, lv, 30);
        strncpy(interface.dst_virtual_ip, rv, 30);
        node.interface[node.interface_size] = interface;
        node.interface_size = node.interface_size + 1;
    }
    printf("%d\n", node.interface_size);
    graph.node[nodeNum] = node;
    return graph;
}

void dijkstra(int G[64][64])
{
    int n = graph.size;
    int startnode = nodeNum;
	int cost[64][64];
	int visited[64],count,mindistance,nextnode,i,j;
	
	//pred[] stores the predecessor of each node
	//count gives the number of nodes seen so far
	//create the cost matrix
	for(i=0;i<n;i++)
		for(j=0;j<n;j++)
			if(G[i][j]==0)
				cost[i][j]=64;
			else
				cost[i][j]=G[i][j];
	
	//initialize pred[],distance[] and visited[]
	for(i=0;i<n;i++)
	{
		distance[i]=cost[startnode][i];
		pred[i]=startnode;
		visited[i]=0;
	}
	
	distance[startnode]=0;
	visited[startnode]=1;
	count=1;
	
	while(count<n-1)
	{
		mindistance=64;
		
		//nextnode gives the node at minimum distance
		for(i=0;i<n;i++){
			if(distance[i]<mindistance&&!visited[i])
			{
				mindistance=distance[i];
				nextnode=i;
			}
        }
			
			//check if a better path exists through nextnode			
        visited[nextnode]=1;
        for(i=0;i<n;i++){
            if(!visited[i]){
                if(mindistance+cost[nextnode][i]<distance[i])
                {
                    distance[i]=mindistance+cost[nextnode][i];
                    pred[i]=nextnode;
                }
            }
        }
		count++;
	}
}

void find_forwarding_table(){
    int g[64][64] = {0};
    for (int i = 0; i < graph.size; i++)
    {
        if (graph.node[i].is_on)
        {
            for (int j = 0; j < graph.node[i].interface_size; j++)
            {
                if (graph.node[i].interface[j].is_up)
                {
                    g[i][graph.node[i].interface[j].dest_id] = 1;
                }
            }
        }
    }
    for (int i = 0; i < graph.size; i++)
    {
        for (int j = 0; j < graph.size; j++)
        {
            if (g[i][j] == 1 && g[j][i] == 0)
            {
                g[i][j] = 0;
            }
        } 
    }
    //printf("hello again\n");
    // for (int i = 0; i < graph.size; i++)
    // {
    //     for (int j = 0; j < graph.size; j++)
    //     {
    //         printf("%d ", g[i][j]);
    //     }
    //     printf("\n");
    // }
    
    dijkstra(g);
    
}

char* find_source(int m){
    char source_virtual_ip[30];
    while (pred[m] != nodeNum)
    {
        m = pred[m];
    }
    for (int i = 0; i < graph.node[nodeNum].interface_size; i++)
    {
        if(graph.node[nodeNum].interface[i].dest_id == m){
            return graph.node[nodeNum].interface[i].src_virtual_ip;
        }
    }
    return source_virtual_ip;
}


bool is_empty(){
    for (int i = 0; i < graph.size; i++)
    {
        if (graph.node[i].is_on)
        {
            printf("no\n");
            return false;
        }  
    }
    return true;
    
}



#endif
