#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

// Pobranie aktualnego czasu w milisekundach
uint64_t current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
}

// Prosty parser pojedynczej linii JSON (format JSON Lines)
int parse_json_line(const char *line, RadarData *out) {
    const char *p;
    if ((p = strstr(line, "\"target_id\""))) sscanf(p, "\"target_id\"%*[^0-9-]%d", &out->target_id);
    else return 0;

    if ((p = strstr(line, "\"azimuth\""))) sscanf(p, "\"azimuth\"%*[^0-9.-]%lf", &out->azimuth_deg);
    if ((p = strstr(line, "\"elevation\""))) sscanf(p, "\"elevation\"%*[^0-9.-]%lf", &out->elevation_deg);
    if ((p = strstr(line, "\"range\""))) sscanf(p, "\"range\"%*[^0-9.-]%lf", &out->range_m);
    if ((p = strstr(line, "\"velocity\""))) sscanf(p, "\"velocity\"%*[^0-9.-]%lf", &out->velocity_mps);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        printf("Użycie: %s <radar_id> <x> <y> <z> <plik_json>\n", argv[0]);
        printf("Przykład: %s 1 100.0 250.0 10.0 radar_data.json\n", argv[0]);
        return 1;
    }

    int radar_id = atoi(argv[1]);
    double x = atof(argv[2]);
    double y = atof(argv[3]);
    double z = atof(argv[4]);
    const char *json_path = argv[5];

    FILE *fp = fopen(json_path, "r");
    if (!fp) { perror("fopen"); return 1; }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); fclose(fp); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_aton("127.0.0.1", &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(sockfd); fclose(fp); return 1;
    }

    // 1. Rejestracja radaru na serwerze
    Packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = MSG_REGISTER;
    pkt.payload.reg.radar_id = radar_id;
    snprintf(pkt.payload.reg.desc, sizeof(pkt.payload.reg.desc), "Radar_%d", radar_id);
    pkt.payload.reg.pos = (Position3D){x, y, z};

    send(sockfd, &pkt, sizeof(Packet), 0);

    char ack[128] = {0};
    if (recv(sockfd, ack, sizeof(ack) - 1, 0) > 0) {
        printf("[RADAR %d] Odpowiedź serwera: %s\n", radar_id, ack);
    }

    // 2. Pętla wysyłania danych z pliku JSON z częstotliwością 10 Hz (co 100 ms)
    char line[512];
    while (1) {
        if (!fgets(line, sizeof(line), fp)) {
            // Zapętlenie pliku JSON po dojściu do końca (symulacja ciągłej pracy)
            rewind(fp);
            continue;
        }

        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_RADAR_DATA;
        pkt.payload.data.radar_id = radar_id;

        if (parse_json_line(line, &pkt.payload.data)) {
            pkt.payload.data.timestamp_ms = current_time_ms();
            if (send(sockfd, &pkt, sizeof(Packet), 0) <= 0) {
                perror("send");
                break;
            }
            usleep(100000); // 100 000 mikrosekund = 100 ms = 10 Hz
        }
    }

    fclose(fp);
    close(sockfd);
    return 0;
}