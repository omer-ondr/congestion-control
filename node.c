#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "node.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <conio.h>
    #include <process.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
#endif

#define DV_INTERVAL 5 
#define INF 999
#define PKT_DV 1
#define PKT_DATA 2

#pragma pack(push, 1)
typedef struct {
    int type;
    char from;
    int vector[MAX_NODES];
    char paths[MAX_NODES][128];
} DVPacket;

typedef struct {
    int type;
    char from;
    char dest;
    char payload[256];
} DataPacket;
#pragma pack(pop)

NodeConfig g_config;
SOCKET g_sockfd;

const char *state_name(CCState state) {
    switch (state) {
        case SLOW_START: return "Slow Start";
        case CONGESTION_AVOIDANCE: return "Congestion Avoidance";
        default: return "Unknown";
    }
}

int load_config(const char *filename, NodeConfig *config) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[MAX_LINE];
    config->neighbor_count = 0;
    config->node_count = 6; 
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, " %c %d", &config->node_id, &config->port) == 2) break;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (config->neighbor_count >= MAX_NEIGHBORS) break;
        Neighbor *n = &config->neighbors[config->neighbor_count];
        if (sscanf(line, " %c %63s %d %d", &n->id, n->ip, &n->port, &n->cost) == 4) {
            config->neighbor_count++;
        }
    }
    fclose(fp);
    return 1;
}

void init_routing_table(NodeConfig *config) {
    for (int i = 0; i < config->node_count; i++) {
        char target_id = 'A' + i;
        config->routing_table[i].dest = target_id;
        if (target_id == config->node_id) {
            config->routing_table[i].cost = 0;
            config->routing_table[i].next_hop = target_id;
            sprintf(config->routing_table[i].path, "%c", target_id);
        } else {
            config->routing_table[i].cost = INF;
            config->routing_table[i].next_hop = '-';
            strcpy(config->routing_table[i].path, "-");
            for (int j = 0; j < config->neighbor_count; j++) {
                if (config->neighbors[j].id == target_id) {
                    config->routing_table[i].cost = config->neighbors[j].cost;
                    config->routing_table[i].next_hop = target_id;
                    sprintf(config->routing_table[i].path, "%c -> %c", config->node_id, target_id);
                    break;
                }
            }
        }
    }
}

void print_routing_table(NodeConfig *config) {
    printf("\n--- Routing Table for Node %c ---\n", config->node_id);
    printf("-------------------------------\n");
    printf("Destination Next Hop Cost Path\n");
    for (int i = 0; i < config->node_count; i++) {
        printf("%-11c %-8c %-4d %s\n", config->routing_table[i].dest, config->routing_table[i].next_hop, config->routing_table[i].cost, config->routing_table[i].path);
    }
    printf("-------------------------------\n");
}

void send_dv_to_neighbors() {
    DVPacket pkt;
    pkt.type = PKT_DV;
    pkt.from = g_config.node_id;
    for (int i = 0; i < g_config.node_count; i++) {
        pkt.vector[i] = g_config.routing_table[i].cost;
        strncpy(pkt.paths[i], g_config.routing_table[i].path, 127);
        pkt.paths[i][127] = '\0';
    }

    for (int i = 0; i < g_config.neighbor_count; i++) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(g_config.neighbors[i].port);
        inet_pton(AF_INET, g_config.neighbors[i].ip, &dest_addr.sin_addr);
        sendto(g_sockfd, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    }
}

void update_routing_table(char from_id, int *neighbor_vector, char neighbor_paths[][128]) {
    int link_cost = INF;
    for (int i = 0; i < g_config.neighbor_count; i++) {
        if (g_config.neighbors[i].id == from_id) { link_cost = g_config.neighbors[i].cost; break; }
    }
    if (link_cost == INF) return;

    int changed = 0;
    for (int i = 0; i < g_config.node_count; i++) {
        if (neighbor_vector[i] != INF) {
            int new_cost = link_cost + neighbor_vector[i];
            if (new_cost < g_config.routing_table[i].cost) {
                g_config.routing_table[i].cost = new_cost;
                g_config.routing_table[i].next_hop = from_id;
                sprintf(g_config.routing_table[i].path, "%c -> %s", g_config.node_id, neighbor_paths[i]);
                changed = 1;
            }
        }
    }
    if (changed) {
        printf("Routing table updated by info from %c\n", from_id);
    }
}

void handle_send_command(char *line) {
    char dest_id, msg[256];
    if (sscanf(line, "send %c %255s", &dest_id, msg) != 2) {
        printf("Usage: send <NodeID> <Message>\n");
        return;
    }
    int idx = dest_id - 'A';
    if (idx < 0 || idx >= g_config.node_count || g_config.routing_table[idx].next_hop == '-') {
        printf("No route to %c\n", dest_id);
        return;
    }

    char next_hop = g_config.routing_table[idx].next_hop;
    printf("[%c] Sending to %c via %c\n", g_config.node_id, dest_id, next_hop);

    DataPacket pkt;
    pkt.type = PKT_DATA;
    pkt.from = g_config.node_id;
    pkt.dest = dest_id;
    strncpy(pkt.payload, msg, 255);

    for (int i = 0; i < g_config.neighbor_count; i++) {
        if (g_config.neighbors[i].id == next_hop) {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(g_config.neighbors[i].port);
            inet_pton(AF_INET, g_config.neighbors[i].ip, &addr.sin_addr);
            sendto(g_sockfd, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&addr, sizeof(addr));
            break;
        }
    }
}

void init_tcp(TCPState *tcp) {
    tcp->state = SLOW_START; tcp->cwnd = 1.0; tcp->ssthresh = 16.0; tcp->dup_ack_count = 0; tcp->round = 0;
}

static void print_tcp_status(TCPState *tcp, const char *event, int ack_no, const char *note) {
    printf("%5d | %-10s | ACK=%-3d | cwnd=%5.2f | ssthresh=%5.2f | %-22s | %s\n",
           tcp->round, event, ack_no, tcp->cwnd, tcp->ssthresh, state_name(tcp->state), note);
}

void on_ack(TCPState *tcp, int ack_no) {
    tcp->round++; tcp->dup_ack_count = 0;
    if (tcp->state == SLOW_START) {
        tcp->cwnd += 1.0; 
        if (tcp->cwnd >= tcp->ssthresh) tcp->state = CONGESTION_AVOIDANCE;
        print_tcp_status(tcp, "ACK", ack_no, "Slow Start increase");
    } else if (tcp->state == CONGESTION_AVOIDANCE) {
        tcp->cwnd += 1.0 / tcp->cwnd; 
        print_tcp_status(tcp, "ACK", ack_no, "Additive increase");
    }
}

void on_duplicate_ack(TCPState *tcp, int ack_no) {
    tcp->round++; tcp->dup_ack_count++;
    if (tcp->dup_ack_count < 3) { 
        print_tcp_status(tcp, "DUPACK", ack_no, "Waiting"); 
        return; 
    }
    
    // TCP Tahoe implementation for 3 Duplicate ACKs
    tcp->ssthresh = tcp->cwnd / 2.0; 
    if (tcp->ssthresh < 2.0) tcp->ssthresh = 2.0;
    tcp->cwnd = 1.0; 
    tcp->state = SLOW_START; 
    tcp->dup_ack_count = 0;
    print_tcp_status(tcp, "DUPACK", ack_no, "3 DUPACKs - Tahoe Reset to 1");
}

void on_timeout(TCPState *tcp) {
    tcp->round++; 
    tcp->ssthresh = tcp->cwnd / 2.0; 
    if (tcp->ssthresh < 2.0) tcp->ssthresh = 2.0;
    tcp->cwnd = 1.0; 
    tcp->dup_ack_count = 0; 
    tcp->state = SLOW_START; 
    print_tcp_status(tcp, "TIMEOUT", 0, "Timeout - Tahoe Reset to 1");
}

int run_event_file(const char *event_file) {
    TCPState tcp; init_tcp(&tcp);
    FILE *fp = fopen(event_file, "r"); if (!fp) return 0;
    char event[32]; int ack_no;
    printf("\nTrace for TCP Tahoe Implementation\n");
    printf("Round | Event      | ACK    | cwnd       | ssthresh   | State                  | Explanation\n");
    printf("------------------------------------------------------------------------------------------------\n");
    while (fscanf(fp, "%31s", event) == 1) {
        if (strcmp(event, "ACK") == 0) { fscanf(fp, "%d", &ack_no); on_ack(&tcp, ack_no); }
        else if (strcmp(event, "DUPACK") == 0) { fscanf(fp, "%d", &ack_no); on_duplicate_ack(&tcp, ack_no); }
        else if (strcmp(event, "TIMEOUT") == 0) on_timeout(&tcp);
    }
    fclose(fp); return 1;
}

#ifdef _WIN32
unsigned __stdcall input_thread(void *arg) {
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "send", 4) == 0) handle_send_command(line);
        else if (strncmp(line, "table", 5) == 0) print_routing_table(&g_config);
        else if (strncmp(line, "exit", 4) == 0) exit(0);
    }
    return 0;
}
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    if (argc < 2) {
        printf("Usage: %s <config_file> [event_file]\n", argv[0]);
        return 1;
    }
    if (!load_config(argv[1], &g_config)) return 1;
    init_routing_table(&g_config);
    
    // If an event file is provided, run the TCP Tahoe simulation
    if (argc == 3) { 
        run_event_file(argv[2]); 
        return 0; 
    }

    g_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET; server.sin_port = htons(g_config.port); server.sin_addr.s_addr = INADDR_ANY;
    bind(g_sockfd, (struct sockaddr*)&server, sizeof(server));

    printf("Node %c listening on port %d. (TCP Tahoe Mode)\n", g_config.node_id, g_config.port);
    printf("Type 'table' to see routing, or 'send <ID> <MSG>' to test connectivity.\n");

#ifdef _WIN32
    _beginthreadex(NULL, 0, input_thread, NULL, 0, NULL);
#endif

    time_t last_dv = 0;
    while (1) {
        time_t now = time(NULL);
        if (now - last_dv >= DV_INTERVAL) { send_dv_to_neighbors(); last_dv = now; }

        fd_set fds; FD_ZERO(&fds); FD_SET(g_sockfd, &fds);
#ifndef _WIN32
        FD_SET(STDIN_FILENO, &fds);
#endif
        struct timeval tv = {1, 0};
        if (select(g_sockfd + 1, &fds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(g_sockfd, &fds)) {
                char buf[4096]; struct sockaddr_in from; int from_len = sizeof(from);
                int len = recvfrom(g_sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &from_len);
                if (len > 0) {
                    int type = *(int*)buf;
                    if (type == PKT_DV) {
                        DVPacket *dv = (DVPacket*)buf; update_routing_table(dv->from, dv->vector, dv->paths);
                    } else if (type == PKT_DATA) {
                        DataPacket *dp = (DataPacket*)buf;
                        if (dp->dest == g_config.node_id) printf("[%c] Received: %s\n", g_config.node_id, dp->payload);
                        else {
                            int idx = dp->dest - 'A'; char next = g_config.routing_table[idx].next_hop;
                            printf("[%c] Forwarding to %c via %c\n", g_config.node_id, dp->dest, next);
                            for (int i = 0; i < g_config.neighbor_count; i++) {
                                if (g_config.neighbors[i].id == next) {
                                    struct sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_port = htons(g_config.neighbors[i].port);
                                    inet_pton(AF_INET, g_config.neighbors[i].ip, &addr.sin_addr);
                                    sendto(g_sockfd, (char*)dp, sizeof(DataPacket), 0, (struct sockaddr*)&addr, sizeof(addr));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
