/* 
 * File:   cc1120_config.h
 * Author: ingek
 *
 * Created on May 25, 2025, 12:50 AM
 */

#ifndef CC1120_CONFIG_H
#define	CC1120_CONFIG_H

#include <stdint.h>

typedef struct {
    uint16_t addr;
    uint8_t  value;
} cc1120_reg_setting_t;

extern const cc1120_reg_setting_t cc1120_config[];
extern const uint16_t cc1120_config_size;


#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* CC1120_CONFIG_H */

