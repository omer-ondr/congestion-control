#ifndef NODE_H
#define NODE_H

#define MAX_NEIGHBORS 16
#define MAX_NODES 26
#define MAX_LINE 256
#define INF 999

typedef enum {
    SLOW_START = 0,
    CONGESTION_AVOIDANCE
} CCState;

typedef struct {
    char id;
    char ip[64];
    int port;
    int cost;
} Neighbor;

typedef struct {
    char dest;
    char next_hop;
    int cost;
    char path[128];
} RouteEntry;

typedef struct {
    char node_id;
    int port;
    Neighbor neighbors[MAX_NEIGHBORS];
    int neighbor_count;      
    RouteEntry routing_table[MAX_NODES];
    int node_count;
} NodeConfig;

typedef struct {
    CCState state;
    double cwnd;
    double ssthresh;
    int dup_ack_count;       
    int round;
} TCPState;

int load_config(const char *filename, NodeConfig *config);
void init_routing_table(NodeConfig *config);
void update_routing_table(char from_id, int *neighbor_vector, char neighbor_paths[][128]);
void print_routing_table(NodeConfig *config);

const char *state_name(CCState state);

void init_tcp(TCPState *tcp);
void on_ack(TCPState *tcp, int ack_no);
void on_duplicate_ack(TCPState *tcp, int ack_no);
void on_timeout(TCPState *tcp);

#endif
