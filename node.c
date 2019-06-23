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

void help_cmd(const char *line) {
    (void) line;

    printf("- help, h: Print this list of commands\n"
	   "- interfaces, li: Print information about each interface, one per line\n"
	   "- routes, lr: Print information about the route to each known destination, one per line\n"
	   "- up [integer]: Bring an interface \"up\" (it must be an existing interface, probably one you brought down)\n"
	   "- down [integer]: Bring an interface \"down\"\n"
	   "- send [ip] [protocol] [payload]: sends payload with protocol=protocol to virtual-ip ip\n"
	   "- q: quit this node\n");
}

// char * toArray(int number){
//     int n = log10(number) + 1;
//     int i;
//     char *numberArray = calloc(n, sizeof(char));
//     for ( i = 0; i < n; ++i, number /= 10 )
//     {
//         numberArray[i] = number % 10;
//     }
//     return numberArray;
// }

void interfaces_cmd(const char *line){
    (void) line;
    //TODO interfaces
    dbg(DBG_ERROR, "interfaces_cmd: NOT YET IMPLEMENTED\n");
}

void routes_cmd(const char *line){
    (void) line;
    //TODO routes
    dbg(DBG_ERROR, "routes_cmd: NOT YET IMPLEMENTED\n");
}

void down_cmd(const char *line){
    unsigned interface;

    if (sscanf(line, "down %u", &interface) != 1) {
        dbg(DBG_ERROR, "syntax error (usage: down [interface])\n");
        return;
    }
    //TODO down
    dbg(DBG_ERROR, "down_cmd: NOT YET IMPLEMENTED\n");
}

void up_cmd(const char *line){
    unsigned interface;

    if (sscanf(line, "up %u", &interface) != 1) {
        dbg(DBG_ERROR, "syntax error (usage: up [interface])\n");
        return;
    }
    //TODO up
    dbg(DBG_ERROR, "up_cmd: NOT YET IMPLEMENTED\n");
}

void send_cmd(const char *line){
    char ip_string[INET_ADDRSTRLEN];
    struct in_addr ip_addr;
    uint8_t protocol;
    int num_consumed;
    char *data;

    if (sscanf(line, "send %s %" SCNu8 "%n", ip_string, &protocol, &num_consumed) != 2) {
	dbg(DBG_ERROR, "syntax error (usage: send [ip] [protocol] [payload])\n");
	return;
    }

    if (inet_pton(AF_INET, ip_string, &ip_addr) == 0) {
        dbg(DBG_ERROR, "syntax error (malformed ip address)\n");
        return;
    }

    data = ((char *)line) + num_consumed + 1;

    if (strlen(data) < 1) {
        dbg(DBG_ERROR, "syntax error (payload unspecified)\n");
        return;
    }
    //TODO send
    dbg(DBG_ERROR, "send_cmd: NOT YET IMPLEMENTED\n");
}

struct {
  const char *command;
  void (*handler)(const char *);
} cmd_table[] = {
  {"help", help_cmd},
  {"h", help_cmd},
  {"interfaces", interfaces_cmd},
  {"i", interfaces_cmd},
  {"routes", routes_cmd},
  {"r", routes_cmd},
  {"down", down_cmd},
  {"up", up_cmd},
  {"send", send_cmd}
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
    int nodeNum;
    int version = 0;
    int ret;
    int ShmID;
    bool delete_shared_memory;
    Routing_table *ShmPTR;
    Graph graph;
    
    ShmPTR = Initialize_Shared_Memory(&ShmID);
    graph = ShmPTR->graph;
    graph = AddNewNode(argv[1], graph, &nodeNum);
    ShmPTR->version = ShmPTR->version + 1;
    ShmPTR->graph = graph;
    while (1) {
        printf("%d\n", ShmPTR->version);
        for (int i = 0; i < ShmPTR->graph.size; i++)
        {
            printf("node %d:\n", i);
            printf("is on :%d\n", ShmPTR->graph.node[i].is_on);
            printf("virtual id :%s\n", ShmPTR->graph.node[i].virtual_ip);
            printf("interface size :%d\n", ShmPTR->graph.node[i].interface_size);
            for (int j = 0; j < ShmPTR->graph.node[i].interface_size; j++)
            {
                printf("interface %d:\n", j);
                printf("is up :%d\n", ShmPTR->graph.node[i].interface[j].is_up);
                printf("destination :%d\n", ShmPTR->graph.node[i].interface[j].dest_id);
            }
            printf("-------------------------------------------------------\n");
        }
        
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
                cmd_table[i].handler(line);
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
    ShmPTR->graph.node[nodeNum].is_on = false;
    delete_shared_memory = is_empty(ShmPTR->graph);
    shmdt(ShmPTR);
    if(delete_shared_memory){
        shmctl(ShmID,IPC_RMID,NULL);
    } 

    printf("\nGoodbye!\n\n");
    return 0;
}
