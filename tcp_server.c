#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

// Stan jednostki głównej (serwera) - baza pod przyszłe TEWA
Position3D server_pos = {0.0, 0.0, 15.0}; // np. punkt (0, 0, 15m)

// Rejestr aktywnych radarów
typedef struct {
    int active;
    RadarRegister info;
    char ip[INET_ADDRSTRLEN];
} RadarNode;

RadarNode radar_registry[MAX_RADARS];
pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// Pomocnicza funkcja gwarantująca odebranie pełnej liczby bajtów w TCP
int recv_all(int sockfd, void *buf, size_t len) {
    size_t total = 0;
    char *p = (char *)buf;
    while (total < len) {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n <= 0) return n; // 0 = rozłączenie, <0 = błąd
        total += n;
    }
    return 1;
}

typedef struct {
    int clientfd;
    struct sockaddr_in addr;
} ClientArgs;

void *handle_client(void *arg) {
    ClientArgs *cargs = (ClientArgs *)arg;
    int clientfd = cargs->clientfd;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cargs->addr.sin_addr, client_ip, sizeof(client_ip));
    free(cargs);

    printf("[SERWER] Nowe połączenie z %s\n", client_ip);

    Packet pkt;
    int registered_slot = -1;

    while (recv_all(clientfd, &pkt, sizeof(Packet)) > 0) {
        if (pkt.type == MSG_REGISTER) {
            pthread_mutex_lock(&registry_mutex);
            for (int i = 0; i < MAX_RADARS; i++) {
                if (!radar_registry[i].active) {
                    radar_registry[i].active = 1;
                    radar_registry[i].info = pkt.payload.reg;
                    strcpy(radar_registry[i].ip, client_ip);
                    registered_slot = i;
                    break;
                }
            }
            pthread_mutex_unlock(&registry_mutex);

            printf("[REJESTRACJA] Radar ID=%d (%s) | Pozycja: (%.1f, %.1f, %.1f) | IP: %s\n",
                   pkt.payload.reg.radar_id, pkt.payload.reg.desc,
                   pkt.payload.reg.pos.x, pkt.payload.reg.pos.y, pkt.payload.reg.pos.z,
                   client_ip);

            const char *ack = "OK: Radar zarejestrowany";
            send(clientfd, ack, strlen(ack) + 1, 0);

        } else if (pkt.type == MSG_RADAR_DATA) {
            RadarData *d = &pkt.payload.data;
            // Tutaj w przyszłości trafi wywołanie algorytmu TEWA z użyciem server_pos i radar_registry
            printf("[DANE 10Hz] Radar %d -> Cel #%d | Az: %6.2f° | El: %5.2f° | Dyst: %8.1fm | V: %6.1fm/s\n",
                   d->radar_id, d->target_id, d->azimuth_deg, d->elevation_deg,
                   d->range_m, d->velocity_mps);
        }
    }

    printf("[SERWER] Rozłączono klienta %s\n", client_ip);
    if (registered_slot != -1) {
        pthread_mutex_lock(&registry_mutex);
        radar_registry[registered_slot].active = 0;
        pthread_mutex_unlock(&registry_mutex);
    }

    close(clientfd);
    return NULL;
}

int main() {
    int sockfd, opt = 1;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sockfd); return 1;
    }

    // Kolejka oczekujących połączeń zwiększona z 1 na MAX_RADARS
    if (listen(sockfd, MAX_RADARS) < 0) {
        perror("listen"); close(sockfd); return 1;
    }

    printf("[SERWER] Jednostka główna uruchomiona na porcie %d. Pozycja serwera: (%.1f, %.1f, %.1f)\n",
           SERVER_PORT, server_pos.x, server_pos.y, server_pos.z);

    while (1) {
        ClientArgs *cargs = malloc(sizeof(ClientArgs));
        socklen_t addr_len = sizeof(cargs->addr);
        cargs->clientfd = accept(sockfd, (struct sockaddr *)&cargs->addr, &addr_len);
        if (cargs->clientfd < 0) {
            perror("accept");
            free(cargs);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, cargs) != 0) {
            perror("pthread_create");
            close(cargs->clientfd);
            free(cargs);
        } else {
            pthread_detach(tid); // Automatyczne zwalnianie zasobów wątku po zakończeniu
        }
    }

    close(sockfd);
    return 0;
}