#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define SERVER_PORT 12345
#define MAX_RADARS 16

typedef enum {
    MSG_REGISTER = 1,    // Rejestracja radaru i jego pozycji
    MSG_RADAR_DATA = 2   // Cykliczne dane pomiarowe (10 Hz)
} MessageType;

// Pozycja w przestrzeni 3D (np. w metrach, układ lokalny ENU)
typedef struct {
    double x;
    double y;
    double z;
} Position3D;

// Dane rejestracyjne radaru (odpowiednik wcześniejszego weaponData)
typedef struct {
    int radar_id;
    char desc[64];
    Position3D pos;
} RadarRegister;

// Przetworzone dane pomiarowe z radaru (czytelne dla algorytmów TEWA)
typedef struct {
    int radar_id;
    int target_id;
    double azimuth_deg;     // Azymut [stopnie]
    double elevation_deg;   // Elewacja [stopnie]
    double range_m;         // Odległość [metry]
    double velocity_mps;    // Prędkość radialna [m/s]
    uint64_t timestamp_ms;  // Znacznik czasu [ms]
} RadarData;

// Główny pakiet przesyłany przez TCP
typedef struct {
    MessageType type;
    union {
        RadarRegister reg;
        RadarData data;
    } payload;
} Packet;

#endif