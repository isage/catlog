#ifndef CATLOG_H
#define CATLOG_H

#include <stdint.h>

typedef struct {
    uint32_t host;
    uint16_t port;
    uint16_t loglevel;
} CatLogConfig_t;

int CatLogReadConfig(uint32_t* host, uint16_t* port, uint16_t* level);
int CatLogUpdateConfig(uint32_t host, uint16_t port, uint16_t level);

#endif // CATLOG_H