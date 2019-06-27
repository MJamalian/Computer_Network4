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
#include <sys/socket.h> 

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "dbg.h"
#include "lnxparse.h"
#include "shm.h"
#include "node.h"
#include "ip.h"

Graph graph;
int nodeNum;
int version;
int version_fd;
int distance[64];
int pred[64];
int fd;

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
        if(myNode.interface[i].is_up)
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
                int n = find_source(i);
                char* src_virtual_ip = graph.node[nodeNum].interface[n].src_virtual_ip;
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
}

bool send_cmd(const char *line){
    if (version_fd < version)
    {
        find_forwarding_table();
        version_fd = version;
    }
    char ip_string[INET_ADDRSTRLEN];
    int protocol;
    char data[1000];
    if (sscanf(line, "send %s %d %s", ip_string, &protocol, &data) != 3) {
	    dbg(DBG_ERROR, "syntax error (usage: send [ip] [protocol] [payload])\n");
	    return false;
    }
    if (strlen(data) < 1) {
        dbg(DBG_ERROR, "syntax error (payload unspecified)\n");
        return false;
    }
    int dest_id = find_node(ip_string);
    if (dest_id == -1)
    {
        printf("There is no route to this ip\n");
        return false;
    }
    if (distance[dest_id]>63)
    {
        printf("there is no route from here to that ip\n");
        return false;
    }
    else{
        int dest_id2 = dest_id;
        while (pred[dest_id] != nodeNum)
        {
            dest_id = pred[dest_id];
        }
        struct sockaddr_in m;
        memset(&m, 0, sizeof(m)); 
        m.sin_family = AF_INET; 
        m.sin_port = htons(graph.node[dest_id].port); 
        m.sin_addr.s_addr = INADDR_ANY;
        ip_packet ip_pct;
        strncpy(ip_pct.payload, data, 1000);
        for (int i = 0; i < graph.node[nodeNum].interface_size; i++)
        {
            if (graph.node[nodeNum].interface[i].dest_id == dest_id)
            {
                strncpy(ip_pct.source_ip, graph.node[nodeNum].interface[i].src_virtual_ip, 30);
            }  
        }
        strncpy(ip_pct.dest_ip, ip_string, 30);
        ip_pct.protocol = protocol;
        sendto(fd,(struct IP*) &ip_pct, sizeof(ip_pct), MSG_CONFIRM, (const struct sockaddr *) &m,  sizeof(m));
        return false;
    }
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
    int dest_num = find_node(ip_string);
    if(dest_num == -1){
        dbg(DBG_ERROR, "ip is not in network.\n");
        return false;
    }
    else{
        int full_distance = distance[dest_num];
        if (full_distance>63)
        {
            printf("there is no route from here to that ip.\n");
            return false;
        }
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
    char* line;
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
    struct sockaddr_in servaddr;
    if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(graph.node[nodeNum].port);
    if ( bind(fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    while (1) {
        fd_set fds;
        int max_fds = (fd>0)?fd:0;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(0, &fds);
        if (select(max_fds + 1,&fds,NULL,NULL,NULL) == -1){
            perror("select:");
            exit(1);
        }
        if(FD_ISSET(fd, &fds)){
            if (version < ShmPTR->version)
            {
                graph = ShmPTR->graph;
                version = ShmPTR->version;
            }
            if (version_fd < version)
            {
                find_forwarding_table();
                version_fd = version;
            }
            int len;
            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            struct IP * temp = malloc(sizeof(struct IP));
            recvfrom(fd, temp, sizeof(*temp),  MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
            char dst[30];
            strncpy(dst, temp->dest_ip, 30);
            bool is_dest = false;
            for (int i = 0; i < graph.node[nodeNum].interface_size; i++)
            {
                if (strcmp(dst, graph.node[nodeNum].interface[i].src_virtual_ip)==0)
                {
                    is_dest = true;
                    if (temp->protocol == 0)
                    {
                        printf("----------------------------\n");
                        printf("new packet arrived\n");
                        printf("source: %s", temp->source_ip);
                        printf("\ndestination: %s\n", temp->dest_ip);
                        printf("data: %s\n", temp->payload);
                        printf("----------------------------\n");
                    }
                    break;
                } 
            }
            if (!is_dest)
            {
                int dest_id = find_node(dst);
                if (dest_id == -1)
                {
                    printf("There is no route to this ip\n");
                    continue;
                }
                int dest_id2 = dest_id;
                while (pred[dest_id] != nodeNum)
                {
                    dest_id = pred[dest_id];
                }
                struct sockaddr_in m;
                memset(&m, 0, sizeof(m)); 
                m.sin_family = AF_INET; 
                m.sin_port = htons(graph.node[dest_id2].port); 
                m.sin_addr.s_addr = INADDR_ANY;

                sendto(fd,(struct IP*) temp, sizeof(*temp), MSG_CONFIRM, (const struct sockaddr *) &m,  sizeof(m));
            }
            
            
        }
        if(FD_ISSET(0, &fds)){
            line = readline("");
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
        }
    }
    ShmPTR->version = ShmPTR->version + 1;
    ShmPTR->graph.node[nodeNum].is_on = false;
    graph = ShmPTR->graph;
    delete_shared_memory = is_empty();
    shmdt(ShmPTR);
    if(delete_shared_memory){
        shmctl(ShmID,IPC_RMID,NULL);
    }
    close(fd);
    printf("\nGoodbye!\n\n");
    return 0;
}
