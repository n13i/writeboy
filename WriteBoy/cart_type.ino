/*
 * GB Cartridge type detection and type-specific register control
 */

#include "types.h"

cart_info_t cart_get_info()
{
  byte header[0x50];

  for (int i = 0; i < 0x50; i++)
  {
    header[i] = io_read_byte(0x0100 + i);
  }

  // cartridge type detection
  byte cartridge_type = header[0x47];
  byte rom_size = header[0x48];
  byte ram_size = header[0x49];

  cart_info_t cart;
  cart.type = cartridge_type;
  cart.rom_size = rom_size;
  cart.ram_size = ram_size;
  cart.mbc = get_mbc_type(cartridge_type);

  cart.rom_banks = 0;
  if (rom_size >= 0x00 && rom_size <= 0x08)
  {
    // 0x00: 2 banks (no banking required)
    // 0x01: 4 banks
    // 0x02: 8 banks
    //   :
    cart.rom_banks = 2 << rom_size;
  }
  else if (rom_size >= 0x52 && rom_size <= 0x54)
  {
    // these aren't exist?
    // cf. https://gekkio.fi/blog/2015-02-14-mooneye-gb-gameboy-cartridge-types.html

    // 0x52: 72 banks (0x52 = 0b01010010, 72 = 0b01001000 = 0b01000000 | (0b00000010 << (0b01010010 & 0b00000111))
    // 0x53: 80 banks (0x53 = 0b01010011, 80 = 0b01010000 = 0b01000000 | (0b00000010 << (0b01010011 & 0b00000111))
    // 0x54: 96 banks (0x54 = 0b01010100, 96 = 0b01100000 = 0b01000000 | (0b00000010 << (0b01010100 & 0b00000111))
    cart.rom_banks = 0x40 | (0x02 << (rom_size & 0x07));
  }
  cart.rom_total_bytes = cart.rom_banks * 0x4000;

  cart.ram_banks = 1;
  cart.ram_bytes_per_bank = 0x2000; // 8kbytes
  if (ram_size == 0x00)
  {
    if (cartridge_type == 0x06) // MBC2+BATTERY
    {
      cart.ram_bytes_per_bank = 0x200; // 512bytes (but only lower 4 bits are used)
    }
    else
    {
      cart.ram_banks = 0;
    }
  }
  else if (ram_size == 0x01)
  {
    cart.ram_bytes_per_bank = 0x800; // 2kbytes
  }
  else if (ram_size == 0x02)
  {
    // 8kbytes
  }
  else if (ram_size == 0x03)
  {
    cart.ram_banks = 4; // 8kbytes * 4banks
  }
  else if (ram_size == 0x04)
  {
    cart.ram_banks = 16; // 8kbytes * 16banks
  }
  else if (ram_size == 0x05)
  {
    cart.ram_banks = 8; // 8kbytes * 8banks
  }
  else
  {
    cart.ram_banks = 0;
  }
  cart.ram_total_bytes = cart.ram_banks * cart.ram_bytes_per_bank;

  return cart;
}

mbc_type_t get_mbc_type(byte cartridge_type)
{
  // http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
  byte c = cartridge_type;

  if (c == 0x00 || c == 0x08 || c == 0x09)
  {
    return NONE;
  }

  if (c == 0x01 || c == 0x02 || c == 0x03 || c == 0xff)
  {
    // 0xff: HuC1 is treated as MBC1
    return MBC1;
  }

  if (c == 0x05 || c == 0x06)
  {
    return MBC2;
  }

/*
  if (c == 0x0b || c == 0x0c || c == 0x0d)
  {
    return MMM01;
  }
*/

  if (c == 0x0f || c == 0x10 || c == 0x11 || c == 0x12 || c == 0x13 || c == 0xfe)
  {
    // 0xfe: HuC-3 is treated as MBC3 (but untested)
    return MBC3;
  }

  if (c == 0x19 || c == 0x1a || c == 0x1b || c == 0x1c || c == 0x1d || c == 0x1e || c == 0xfc)
  {
    // 0xfc: Pocket Camera is treated as MBC5
    return MBC5;
  }

/*
  if (c == 0x20)
  {
    return MBC6;
  }
*/

  if (c == 0x22)
  {
    return MBC7;
  }

/*
  if (c == 0xfd)
  {
    return TAMA5;
  }

  if (c == 0xfe)
  {
    return HUC3;
  }

  if (c == 0xff)
  {
    return HUC1;
  }
*/

  return UNKNOWN;
}

void cart_rom_bank_select(cart_info_t info, unsigned int bank)
{
  if (bank < 1) { return; }

  if (info.mbc == MBC1)
  {
    // MBC1: max 125 banks (banks 0x20, 0x40, 0x60 are unsupported)

    if (bank == 0x20 || bank == 0x40 || bank == 0x60)
    {
      // FIXME: how should I do?
    }

    // ROM banking mode
    io_write_byte(0x6000, 0x00);
    // upper 2 bits of ROM bank number
    io_write_byte(0x4000, (byte)((bank >> 5) & 0x0003));
    // lower 5 bits of ROM bank number
    io_write_byte(0x2000, (byte)(bank & 0x001f));
  }
  else if (info.mbc == MBC2)
  {
    // MBC2: max 16 banks

    io_write_byte(0x2100, (byte)(bank & 0x000f));
  }
  else if (info.mbc == MBC3)
  {
    // MBC3: max 128 banks

    io_write_byte(0x2000, (byte)(bank & 0x007f));
  }
  else if (info.mbc == MBC5 || info.mbc == MBC7)
  {
    // MBC5: max 512 banks

    // High bit of ROM bank number
    if (bank > 0xff)
    {
      io_write_byte(0x3000, (byte)((bank >> 8) & 0x0001));
    }
    // Low 8 bits of ROM bank number
    io_write_byte(0x2000, (byte)(bank & 0x00ff));
  }
}

void cart_ram_bank_select(cart_info_t info, unsigned int bank)
{
  if (info.mbc == MBC1 || info.mbc == MBC3)
  {
    if (info.mbc == MBC1)
    {
      // RAM banking mode
      io_write_byte(0x6000, 0x01);
    }
    // RAM bank number
    io_write_byte(0x4000, (byte)(bank & 0x0003));
  }
  else if (info.mbc == MBC5 || info.mbc == MBC7)
  {
    // RAM bank number
    io_write_byte(0x4000, (byte)(bank & 0x000f));
  }
}

boolean cart_is_gbmc()
{
  cart_gbmc_enable_np_registers();
  byte b = io_read_byte(0x13f);
  cart_gbmc_disable_np_registers();

  return (b == 0xa5);
}


/*
 * GBMC commands
 * https://forums.nesdev.com/viewtopic.php?f=12&t=11453&start=147
 */

// CMD_04h: Map Entire
void cart_gbmc_map_entire()
{
  io_write_byte(0x0120, 0x04);
  io_write_byte(0x013f, 0xa5);
}

// CMD_05h: Map menu
void cart_gbmc_map_menu()
{
  io_write_byte(0x0120, 0x05);
  io_write_byte(0x013f, 0xa5);
}

// CMD_08h: Undo Wakeup
void cart_gbmc_disable_np_registers()
{
  io_write_byte(0x0120, 0x08);
  io_write_byte(0x013f, 0xa5);
}

// CMD_09h: Wakeup
void cart_gbmc_enable_np_registers()
{
  io_write_byte(0x0120, 0x09);
  io_write_byte(0x0121, 0xaa);
  io_write_byte(0x0122, 0x55);
  io_write_byte(0x013f, 0xa5);
}

// CMD_0Ah: Wrpot.Step1
void cart_gbmc_disable_write_protect()
{
  // CMD_0Ah: Wrpot.Step1
  io_write_byte(0x0120, 0x0a);
  io_write_byte(0x0125, 0x62);
  io_write_byte(0x0126, 0x04);
  io_write_byte(0x013f, 0xa5);

  // CMD_02h: Wrpot.Step2
  io_write_byte(0x0120, 0x02);
  io_write_byte(0x013f, 0xa5);
}

// CMD_03h: Undo.Step2
void cart_gbmc_undo_step2()
{
  io_write_byte(0x0120, 0x03);
  io_write_byte(0x013f, 0xa5);
}
// CMD_0Fh: Write Byte
void cart_gbmc_write_byte(unsigned int addr, byte data)
{
  io_write_byte(0x0120, 0x0f);
  io_write_byte(0x0125, (byte)((addr >> 8) & 0xff));
  io_write_byte(0x0126, (byte)(addr & 0xff));
  io_write_byte(0x0127, data);
  io_write_byte(0x013f, 0xa5);
}

// CMD_10h: Disable writes to MBC registers
void cart_gbmc_disable_mbc_registers()
{
  io_write_byte(0x0120, 0x10);
  io_write_byte(0x013f, 0xa5);
}

// CMD_11h: Re-Enable writes to MBC registers
void cart_gbmc_enable_mbc_registers()
{
  io_write_byte(0x0120, 0x11);
  io_write_byte(0x013f, 0xa5);
}

// CMD_8xh: Map x with Reset
void cart_gbmc_map_game_with_reset(byte game)
{
  io_write_byte(0x0120, 0x80 | (game & 0x07));
  io_write_byte(0x013f, 0xa5);
}

// CMD_Cxh: Map x without Reset
void cart_gbmc_map_game_without_reset(byte game)
{
  io_write_byte(0x0120, 0xc0 | (game & 0x07));
  io_write_byte(0x013f, 0xa5);
}

// before use:
// - select odd bank (ex. bank 0x01)
// - disable mbc registers
void cart_gbmc_send_flash_cmd(byte cmd)
{
  io_write_byte(0x5555, 0xaa);
  io_write_byte(0x2aaa, 0x55);
  io_write_byte(0x5555, cmd);
}
