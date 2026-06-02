# 1 "cc1120_config.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 285 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include/language_support.h" 1 3
# 2 "<built-in>" 2
# 1 "cc1120_config.c" 2
# 1 "./cc1120_config.h" 1
# 11 "./cc1120_config.h"
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 1 3



# 1 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/musl_xc8.h" 1 3
# 5 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 2 3
# 26 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 1 3
# 133 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned short uintptr_t;
# 148 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 3
typedef short intptr_t;
# 164 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 3
typedef signed char int8_t;




typedef short int16_t;




typedef __int24 int24_t;




typedef long int32_t;





typedef long long int64_t;
# 194 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 3
typedef long long intmax_t;





typedef unsigned char uint8_t;




typedef unsigned short uint16_t;




typedef __uint24 uint24_t;




typedef unsigned long uint32_t;





typedef unsigned long long uint64_t;
# 235 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned long long uintmax_t;
# 27 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 2 3

typedef int8_t int_fast8_t;

typedef int64_t int_fast64_t;


typedef int8_t int_least8_t;
typedef int16_t int_least16_t;

typedef int24_t int_least24_t;
typedef int24_t int_fast24_t;

typedef int32_t int_least32_t;

typedef int64_t int_least64_t;


typedef uint8_t uint_fast8_t;

typedef uint64_t uint_fast64_t;


typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;

typedef uint24_t uint_least24_t;
typedef uint24_t uint_fast24_t;

typedef uint32_t uint_least32_t;

typedef uint64_t uint_least64_t;
# 148 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/bits/stdint.h" 1 3
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
# 149 "C:\\Program Files\\Microchip\\xc8\\v3.00\\pic\\include\\c99/stdint.h" 2 3
# 12 "./cc1120_config.h" 2

typedef struct {
    uint16_t addr;
    uint8_t value;
} cc1120_reg_setting_t;

extern const cc1120_reg_setting_t cc1120_config[];
extern const uint16_t cc1120_config_size;
# 2 "cc1120_config.c" 2
# 565 "cc1120_config.c"
const cc1120_reg_setting_t cc1120_config[] = {
    {0x0000, 0xB0},
    {0x0001, 0x06},
    {0x0002, 0xB0},
    {0x0003, 0x06},
    {0x0004, 0x93},
    {0x0005, 0x0B},
    {0x0006, 0x51},
    {0x0007, 0xDE},
    {0x0008, 0x08},
    {0x0009, 0x17},
    {0x000A, 0xEB},
    {0x000B, 0x2C},
    {0x000C, 0x1C},
    {0x000D, 0x14},
    {0x000E, 0x2A},
    {0x000F, 0x40},
    {0x0010, 0x46},
    {0x0011, 0x42},
    {0x0012, 0x46},
    {0x0013, 0x05},
    {0x0014, 0x8E},
    {0x0015, 0xB8},
    {0x0016, 0x52},
    {0x0017, 0x20},
    {0x0018, 0x19},
    {0x0019, 0x00},
    {0x001A, 0x91},
    {0x001B, 0x20},
    {0x001C, 0xA9},
    {0x001D, 0xCF},
    {0x001E, 0x00},
    {0x001F, 0x00},
    {0x0020, 0x0B},
    {0x0021, 0x1A},
    {0x0022, 0x08},
    {0x0023, 0x21},
    {0x0024, 0x00},
    {0x0025, 0x00},
    {0x0026, 0x00},
    {0x0027, 0x04},
    {0x0028, 0x20},
    {0x0029, 0x0F},
    {0x002A, 0x00},
    {0x002B, 0x7F},
    {0x002C, 0x56},
    {0x002D, 0x7C},
    {0x2E, 0xFF},
    {0x2F00, 0x00},
    {0x2F01, 0x22},
    {0x2F02, 0x0B},
    {0x2F03, 0x00},
    {0x2F04, 0x00},
    {0x2F05, 0x00},
    {0x2F06, 0x01},
    {0x2F07, 0x00},
    {0x2F08, 0x00},
    {0x2F09, 0x00},
    {0x2F0A, 0x00},
    {0x2F0B, 0x00},
    {0x2F0C, 0x69},
    {0x2F0D, 0xA0},
    {0x2F0E, 0x00},
    {0x2F0F, 0x02},
    {0x2F10, 0xA6},
    {0x2F11, 0x04},
    {0x2F12, 0x00},
    {0x2F13, 0x5F},
    {0x2F14, 0x00},
    {0x2F15, 0x20},
    {0x2F16, 0x40},
    {0x2F17, 0x0E},
    {0x2F18, 0x28},
    {0x2F19, 0x03},
    {0x2F1A, 0x00},
    {0x2F1B, 0x33},
    {0x2F1C, 0xFF},
    {0x2F1D, 0x17},

    {0x2F1F, 0x50},
    {0x2F20, 0x6E},
    {0x2F21, 0x14},
    {0x2F22, 0xAC},
    {0x2F23, 0x14},
    {0x2F24, 0x00},
    {0x2F25, 0x00},
    {0x2F26, 0x00},
    {0x2F27, 0xB4},
    {0x2F28, 0x00},
    {0x2F29, 0x02},
    {0x2F2A, 0x00},
    {0x2F2B, 0x00},
    {0x2F2C, 0x10},
    {0x2F2D, 0x00},
    {0x2F2E, 0x00},
    {0x2F2F, 0x01},
    {0x2F30, 0x01},
    {0x2F31, 0x01},
    {0x2F32, 0x0E},
    {0x2F33, 0xA0},
    {0x2F34, 0x03},
    {0x2F35, 0x01},
    {0x2F36, 0x03},

    {0x2F38, 0x00},
    {0x2F39, 0x00},





};

const uint16_t cc1120_config_size = sizeof(cc1120_config) / sizeof(cc1120_config[0]);
