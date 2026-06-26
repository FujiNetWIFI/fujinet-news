#ifndef APPKEY_H
#define APPKEY_H

#include "fujinet-fuji.h"

#define APPKEY_CREATOR_ID 0xB0C1
#define APPKEY_APP_ID     0x02
#define COCO_NEWS_APP_KEY 0x01

bool read_appkey(uint8_t *buffer, uint8_t max_len, uint8_t *out_len);
bool write_appkey(const uint8_t *buffer, uint8_t len);

void settings_load(void);
void settings_save(void);

#endif /* APPKEY_H */
