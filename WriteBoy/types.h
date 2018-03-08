#pragma once

typedef enum mbc_type {
    NONE  = 0x00,
    MBC1  = 0x01,
    MBC2  = 0x02,
    MBC3  = 0x03,
    MBC5  = 0x05,
    MBC6  = 0x06,
    MBC7  = 0x07,
    MMM01 = 0x11,
    TAMA5 = 0xe5,
    HUC1  = 0xf1,
    HUC3  = 0xf3,
    UNKNOWN = 0xff
} mbc_type_t;

typedef struct cart_info {
    byte type;
    byte rom_size;
    byte ram_size;
    mbc_type_t mbc;
    unsigned long rom_total_bytes;
    unsigned long ram_total_bytes;
    unsigned int rom_banks;
    unsigned int ram_banks;
    unsigned int ram_bytes_per_bank;
} cart_info_t;
