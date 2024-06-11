#ifndef CATLOG_H
#define CATLOG_H

#include <stdint.h>

typedef struct {
    uint32_t host;
    uint16_t port;
    uint16_t loglevel;
    uint8_t net;
} CatLogConfig_t;

int CatLogReadConfig(uint32_t* host, uint16_t* port, uint16_t* level, uint8_t* net);
int CatLogUpdateConfig(uint32_t host, uint16_t port, uint16_t level, uint8_t net);

#endif // CATLOG_H