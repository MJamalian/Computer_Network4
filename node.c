#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <math.h>

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "dbg.h"
#include "lnxparse.h"
#include "shm.h"
#include "node.h"

Graph graph;
int nodeNum;
int version;
int version_fd;
int distance[64];
int pred[64];

bool help_cmd(const char *line) {
    (void) line;

    printf("- help, h: Print this list of commands\n"
	   "- interfaces, li: Print information about each interface, one per line\n"
	   "- routes, lr: Print information about the route to each known destination, one per line\n"
	   "- up [integer]: Bring an interface \"up\" (it must be an existing interface, probably one you brought down)\n"
	   "- down [integer]: Bring an interface \"down\"\n"
	   "- send [ip] [protocol] [payload]: sends payload with protocol=protocol to virtual-ip ip\n"
	   "- q: quit this node\n");
    return false;
}

bool interfaces_cmd(const char *line){
    (void) line;
    Node myNode = graph.node[nodeNum];
    printf("id      rem             loc\n");
    for (int i = 0; i < myNode.interface_size; i++)
    {
        printf("%d       %s     %s\n", i , myNode.interface[i].dst_virtual_ip, myNode.interface[i].src_virtual_ip);
    }
    return false;
}
bool routes_cmd(const char *line){
    (void) line;
    if (version_fd < version)
    {
        find_forwarding_table();
        version_fd = version;
    }
    printf("cost      rem             loc\n");
    for (int i = 0; i < graph.size; i++)
    {
        if (i == nodeNum)
        {
            for (int j = 0; j < graph.node[i].interface_size; j++)
            {
                if (graph.node[i].interface[j].is_up)
                {
                    printf("0       %s     %s\n", graph.node[i].interface[j].src_virtual_ip, graph.node[i].interface[j].src_virtual_ip);
                }
            } 
        }
        else
        {
            if (distance[i]<64)
            {
                char* src_virtual_ip = find_source(i);
                for (int j = 0; j < graph.node[i].interface_size; j++)
                {
                    if (graph.node[i].interface[j].is_up)
                    {
                        printf("%d       %s     %s\n",distance[i], graph.node[i].interface[j].src_virtual_ip, src_virtual_ip);
                    }
                } 
            }
            
        }
    }
    // int j, i;
    // int n = graph.size;
    // int startnode = nodeNum;
    // for(i=0;i<n;i++){
	// 	if(i!=startnode)
	// 	{
	// 		printf("\nDistance of node%d=%d",i,distance[i]);
	// 		printf("\nPath=%d",i);
			
	// 		j=i;
	// 		do
	// 		{
	// 			j=pred[j];
	// 			printf("<-%d",j);
	// 		}while(j!=startnode);
	//     }
    //     printf("\n");
    // }


    //dbg(DBG_ERROR, "routes_cmd: NOT YET IMPLEMENTED\n");
    return false;
}

bool down_cmd(const char *line){
    int interface;

    if (sscanf(line, "down %u", &interface) != 1) {
        dbg(DBG_ERROR, "syntax error (usage: down [interface])\n");
        return false;
    }

    //TODO down
    Node myNode = graph.node[nodeNum];
    if (myNode.interface_size > interface)
    {
        if (myNode.interface[interface].is_up == true)
        {
            myNode.interface[interface].is_up = false;
            graph.node[nodeNum] = myNode;
            printf("interface %d is now disabled\n", interface);
            return true;
        }
        else
        {
            printf("interface %d is down already\n", interface);
            return false;
        }
        
    }
    else
    {
        printf("interface %d out of bounds : (0 to %d)\n", interface, myNode.interface_size - 1);
        return false;
    }
    //dbg(DBG_ERROR, "down_cmd: NOT YET IMPLEMENTED\n");
    return false;
}

bool up_cmd(const char *line){
    int interface;

    if (sscanf(line, "up %d", &interface) != 1) {
        dbg(DBG_ERROR, "syntax error (usage: up [interface])\n");
        return false;
    }
    //TODO up
    Node myNode = graph.node[nodeNum];
    if (myNode.interface_size > interface)
    {
        if (myNode.interface[interface].is_up == false)
        {
            myNode.interface[interface].is_up = true;
            graph.node[nodeNum] = myNode;
            printf("interface %d is now enabled", interface);
            return true;
        }
        else
        {
            printf("interface %d is up already", interface);
            return false;
        }
        
    }
    else
    {
        printf("interface %d out of bounds : (0 to %d)", interface, myNode.interface_size - 1);
        return false;
    }
    return false;
    
    //dbg(DBG_ERROR, "up_cmd: NOT YET IMPLEMENTED\n");
}

bool send_cmd(const char *line){
    char ip_string[INET_ADDRSTRLEN];
    struct in_addr ip_addr;
    uint8_t protocol;
    int num_consumed;
    char *data;

    if (sscanf(line, "send %s %" SCNu8 "%n", ip_string, &protocol, &num_consumed) != 2) {
	    dbg(DBG_ERROR, "syntax error (usage: send [ip] [protocol] [payload])\n");
	    return false;
    }

    if (inet_pton(AF_INET, ip_string, &ip_addr) == 0) {
        dbg(DBG_ERROR, "syntax error (malformed ip address)\n");
        return false;
    }

    data = ((char *)line) + num_consumed + 1;

    if (strlen(data) < 1) {
        dbg(DBG_ERROR, "syntax error (payload unspecified)\n");
        return false;
    }
    //TODO send
    dbg(DBG_ERROR, "send_cmd: NOT YET IMPLEMENTED\n");
    return false;
}
bool traceroute_cmd(const char *line){
    if (version_fd < version)
    {
        find_forwarding_table();
        version_fd = version;
    }
    char ip_string[30];
    if (sscanf(line, "traceroute %s", ip_string) != 1) {
        dbg(DBG_ERROR, "syntax error (usage: traceroute [ip])\n");
        return false;
    }
    bool out = false;
    int dest_num = -1;
    for (int i = 0; i < graph.size; i++)
    {
        for (int j = 0; j < graph.node[i].interface_size; j++)
        {
            if(strcmp(graph.node[i].interface[j].src_virtual_ip, ip_string) == 0){
                out = true;
                dest_num = i;
                break;
            }
        }
        if(out)
            break;
    }
    if(dest_num == -1){
        dbg(DBG_ERROR, "ip is not in network.\n");
        return false;
    }
    else{
        int full_distance = distance[dest_num];
        char route_ip[full_distance+1][30];
        int new_node_num;
        strncpy(route_ip[0], ip_string, 30);
        for (int i = 1; i < full_distance+1; i++)
        {
            new_node_num = pred[dest_num];
            for (int j = 0; j < graph.node[new_node_num].interface_size; j++)
            {
                if (graph.node[new_node_num].interface[j].dest_id == dest_num)
                {
                    strncpy(route_ip[i], graph.node[new_node_num].interface[j].src_virtual_ip, 30);
                    dest_num = new_node_num;
                    break;
                }   
            } 
        }
        printf("traceroute from %s to %s\n", route_ip[full_distance], route_ip[0]);
        for (int i = full_distance; i > -1; i--)
        {
            printf("%s\n", route_ip[i]);
        }
        printf("traceroute finished in %d hops\n", full_distance + 1);
    }
    
    return false;
}

struct {
  const char *command;
  bool (*handler)(const char *);
} cmd_table[] = {
  {"help", help_cmd},
  {"h", help_cmd},
  {"interfaces", interfaces_cmd},
  {"i", interfaces_cmd},
  {"routes", routes_cmd},
  {"r", routes_cmd},
  {"down", down_cmd},
  {"up", up_cmd},
  {"send", send_cmd},
  {"traceroute", traceroute_cmd}
};
int main(int argc, char **argv){
    if (argc != 2) {
        dbg(DBG_ERROR, "usage: %s <linkfile>\n", argv[0]);
        return -1;
    }
    #ifdef READLINE
        char* line;
        rl_bind_key('\t', rl_complete);
    #else
        char line[LINE_MAX];
    #endif

    char cmd[LINE_MAX];
    unsigned i;
    int ret;
    int ShmID;
    bool delete_shared_memory;
    Routing_table *ShmPTR;
    version = 0;
    version_fd = 0;
    
    ShmPTR = Initialize_Shared_Memory(&ShmID);
    graph = ShmPTR->graph;
    graph = AddNewNode(argv[1]);
    version = ShmPTR->version + 1;
    ShmPTR->version = version;
    ShmPTR->graph = graph;
    while (1) {
        // printf("%d\n", ShmPTR->version);
        // for (int i = 0; i < ShmPTR->graph.size; i++)
        // {
        //     printf("node %d:\n", i);
        //     printf("is on :%d\n", ShmPTR->graph.node[i].is_on);
        //     printf("virtual id :%s\n", ShmPTR->graph.node[i].virtual_ip);
        //     printf("interface size :%d\n", ShmPTR->graph.node[i].interface_size);
        //     for (int j = 0; j < ShmPTR->graph.node[i].interface_size; j++)
        //     {
        //         printf("interface %d:\n", j);
        //         printf("is up :%d\n", ShmPTR->graph.node[i].interface[j].is_up);
        //         printf("destination :%d\n", ShmPTR->graph.node[i].interface[j].dest_id);
        //     }
        //     printf("-------------------------------------------------------\n");
        // }
        
        #ifdef READLINE
            if (!(line = readline("> "))) break;
        #else
            dbg(DBG_ERROR, "> "); (void)fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) break;
            if (strlen(line) > 0 && line[strlen(line)-1] == '\n')
                line[strlen(line)-1] = 0;
        #endif
        ret = sscanf(line, "%s", cmd);
        if (ret != 1) {
            if (ret != EOF) help_cmd(line);
            continue;
        }
        if (version < ShmPTR->version)
        {
            graph = ShmPTR->graph;
            version = ShmPTR->version;
        }
        if (!strcmp(cmd, "q")) break;

        for (i=0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++){
            if (!strcmp(cmd, cmd_table[i].command)){
                bool change = cmd_table[i].handler(line);
                if (change == true)
                {
                    version++;
                    ShmPTR->version = version;
                    ShmPTR->graph = graph;
                }
                
                break;
            }
        }

        if (i == sizeof(cmd_table) / sizeof(cmd_table[0])){
            dbg(DBG_ERROR, "error: no valid command specified\n");
            help_cmd(line);
            continue;
        }

        #ifdef READLINE
            add_history(line);
            free(line);
        #endif
    }
    ShmPTR->version = ShmPTR->version + 1;
    ShmPTR->graph.node[nodeNum].is_on = false;
    graph = ShmPTR->graph;
    delete_shared_memory = is_empty();
    shmdt(ShmPTR);
    if(delete_shared_memory){
        shmctl(ShmID,IPC_RMID,NULL);
    } 
    //shmctl(ShmID,IPC_RMID,NULL);

    printf("\nGoodbye!\n\n");
    return 0;
}
