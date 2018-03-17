/*
 * WriteBoy - An Arduino based ROM image writer for GB Memory Cartridge
 * 
 * This software is licensed under a
 * Creative Commons Attribution-NonCommercial 4.0 International license.
 * https://creativecommons.org/licenses/by-nc/4.0/
 *
 * (C) 2018 n13i
 */

const int CMD_MAXLEN = 16;

const int READ_BUF = 256;
const int WRITE_BUF = 128;

#include "types.h"

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  io_setup();

  Serial.begin(115200);
}

void loop()
{
  while(Serial.available() <= 0)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  int count = 0;
  char cmd[CMD_MAXLEN+1];
  while(Serial.available() > 0)
  {
    char c = Serial.read();
    if(c == '\n') break;
    if(count < CMD_MAXLEN)
    {
      cmd[count] = c;
      count++;
    }
  } 
  cmd[count] = '\0';

  if(strstr(cmd, "HEADER"))
  {
    dump_header();
  }
  else if(strstr(cmd, "DUMP ROM"))
  {
    dump_rom();
  }
  else if(strstr(cmd, "DUMP SRAM"))
  {
    int title = 0;
    if (cmd[9] == ' ' && cmd[11] == '\0')
    {
      if (cmd[10] >= '1' && cmd[10] <= '7')
      {
        title = (cmd[10] - '1') + 1;
      }
      else
      {
        Serial.print("-ERR unknown title number\n");
        return;
      }
    }
    dump_sram(title);
  }
  else if(strstr(cmd, "WRITE SRAM"))
  {
    int title = 0;
    if (cmd[10] == ' ' && cmd[12] == '\0')
    {
      if (cmd[11] >= '1' && cmd[11] <= '7')
      {
        title = (cmd[11] - '1') + 1;
      }
      else
      {
        Serial.print("-ERR unknown title number\n");
        return;
      }
    }
    write_sram(title);
  }
  else if(strstr(cmd, "DUMP GBMCROM"))
  {
    dump_gbmc_rom();
  }
  else if(strstr(cmd, "DUMP MAPPING"))
  {
    dump_gbmc_mapping();
  }
  else if(strstr(cmd, "DUMP TITLES"))
  {
    dump_gbmc_titles();
  }
  else if(strstr(cmd, "WRITE MAPPING"))
  {
    write_gbmc_mapping();
  }
  else if(strstr(cmd, "WRITE GBMCROM"))
  {
    write_gbmc_rom();
  }
}

void dump_header()
{
  unsigned int addr = 0;
  byte buf[0x50];

  Serial.print("+OK dumping cartridge header\n");

  for (int i = 0; i < 0x50; i++)
  {
    buf[i] = io_read_byte(0x0100 + i);
  }

  Serial.write(buf, 0x50);
}

void dump_rom()
{
  cart_info_t cart = cart_get_info();

  if (cart.rom_banks == 0 || cart.mbc == UNKNOWN)
  {
    Serial.print("-ERR Unknown MBC type\n");
    return;
  }

  Serial.print("+OK Cart type: ");
  Serial.print(cart.type, HEX);
  Serial.print(cart.rom_size, HEX);
  Serial.print(cart.ram_size, HEX);
  Serial.print(", ");
  Serial.print("MBC type: ");
  Serial.print(cart.mbc, HEX);
  Serial.print(", ");
  Serial.print("ROM size: ");
  Serial.print(cart.rom_banks, DEC);
  Serial.print(" banks\n");

  byte buf[READ_BUF];

  unsigned int base_addr = 0x0000;
  for (unsigned int bank = 0; bank < cart.rom_banks; bank++)
  {
    if (bank >= 1)
    {
      base_addr = 0x4000;
      cart_rom_bank_select(cart, bank);
    }

    for (unsigned int addr = 0; addr < 0x4000; addr += READ_BUF)
    {
      for (int i = 0; i < READ_BUF; i++)
      {
        buf[i] = io_read_byte(base_addr + addr + i);
      }
      Serial.write(buf, READ_BUF);
    }
  }
}

void dump_sram(int title)
{
  if (!select_title(title))
  {
    Serial.print("+ERR Failed to select title ");
    Serial.print(title, DEC);
    Serial.print("\n");
    return;
  }

  cart_info_t cart = cart_get_info();

  if (cart.ram_banks == 0 || cart.mbc == UNKNOWN)
  {
    Serial.print("+ERR No SRAM or unknown MBC\n");
    select_title(0);
    return;
  }

  Serial.print("+OK RAM size: ");
  Serial.print(cart.ram_bytes_per_bank, DEC);
  Serial.print(" bytes * ");
  Serial.print(cart.ram_banks, DEC);
  Serial.print(" banks\n");

  byte buf[READ_BUF];

  unsigned int base_addr = 0xa000;
  for (unsigned int bank = 0; bank < cart.ram_banks; bank++)
  {
    cart_ram_bank_select(cart, bank);

    // RAM Enable
    io_write_byte(0x0000, 0x0a);

    for (unsigned int addr = 0; addr < cart.ram_bytes_per_bank; addr += READ_BUF)
    {
      for (int i = 0; i < READ_BUF; i++)
      {
        buf[i] = io_read_byte(base_addr + addr + i);

        if (cart.mbc == MBC2)
        {
          // MBC2: only lower 4 bits are used
          buf[i] = buf[i] | 0xf0;
        }
      }
      Serial.write(buf, READ_BUF);
    }

    // RAM Disable
    io_write_byte(0x0000, 0x00);
  }

  select_title(0);
}

void write_sram(int title)
{
  if (!select_title(title))
  {
    Serial.print("+ERR Failed to select title ");
    Serial.print(title, DEC);
    Serial.print("\n");
    return;
  }

  cart_info_t cart = cart_get_info();

  if (cart.ram_banks == 0 || cart.mbc == UNKNOWN)
  {
    Serial.print("+ERR No SRAM or unknown MBC\n");
    select_title(0);
    return;
  }

  Serial.print("+OK RAM size: ");
  Serial.print(cart.ram_bytes_per_bank, DEC);
  Serial.print(" bytes * ");
  Serial.print(cart.ram_banks, DEC);
  Serial.print(" banks\n");

  byte buf[WRITE_BUF];

  unsigned int base_addr = 0xa000;
  for (unsigned int bank = 0; bank < cart.ram_banks; bank++)
  {
    cart_ram_bank_select(cart, bank);

    // RAM Enable
    io_write_byte(0x0000, 0x0a);

    for (unsigned int addr = 0; addr < cart.ram_bytes_per_bank; addr += WRITE_BUF)
    {
      int buf_count = 0;
      while (buf_count < WRITE_BUF)
      {
        while (Serial.available() <= 0)
        {
          delay(1);
        }

        int value = Serial.read();
        if (value >= 0)
        {
          buf[buf_count] = (byte)value;
          buf_count++;
        }

        if (buf_count >= WRITE_BUF)
        {
          break;
        }
      }

      for (int i = 0; i < WRITE_BUF; i++)
      {
        io_write_byte(base_addr + addr + i, buf[i]);
        Serial.print(".");
      }

      Serial.print("\n");
      Serial.print("+OK Wrote ");
      Serial.print(buf_count, DEC);
      Serial.print(" bytes in bank ");
      Serial.print(bank, DEC);
      Serial.print("/");
      Serial.print(cart.ram_banks - 1, DEC);
      Serial.print("\n");
    }

    // RAM Disable
    io_write_byte(0x0000, 0x00);

    //Serial.print("+OK Wrote RAM bank ");
    //Serial.print(bank, DEC);
    //Serial.print("/");
    //Serial.print(cart.ram_banks - 1, DEC);
    //Serial.print("\n");
  }

  select_title(0);
}

void dump_gbmc_rom()
{
  if (!cart_is_gbmc())
  {
    Serial.print("-ERR cart is not GBMC\n");
    return;
  }

  cart_gbmc_enable_np_registers();
  cart_gbmc_map_entire();
  cart_gbmc_disable_np_registers();

  unsigned int rom_banks = 64;

  Serial.print("+OK Start dumping GBMC ROM 64 banks\n");

  byte buf[READ_BUF];

  unsigned int base_addr = 0x0000;
  for (unsigned int bank = 0; bank < rom_banks; bank++)
  {
    if (bank >= 1)
    {
      base_addr = 0x4000;
      io_write_byte(0x2100, (byte)(bank & 0x003f));
    }

    for (unsigned int addr = 0; addr < 0x4000; addr += READ_BUF)
    {
      for (int i = 0; i < READ_BUF; i++)
      {
        buf[i] = io_read_byte(base_addr + addr + i);
      }
      Serial.write(buf, READ_BUF);
    }
  }
}

void get_gbmc_mapping(byte (&map)[128])
{
  cart_gbmc_enable_np_registers();
  cart_gbmc_disable_write_protect();

  // enable mapping area
  io_write_byte(0x2100, 0x01);
  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0x77);
  cart_gbmc_send_flash_cmd(0x77);
  cart_gbmc_enable_mbc_registers();

  io_write_byte(0x3000, 0x00);
  io_write_byte(0x2100, 0x00);

  for (int i = 0; i < 128; i++)
  {
    map[i] = io_read_byte(i);
  }

  // reset flash
  io_write_byte(0x2100, 0x01);
  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0xf0);
  cart_gbmc_enable_mbc_registers();

  cart_gbmc_map_game_without_reset(0);
  cart_gbmc_disable_np_registers();
}

void dump_gbmc_mapping()
{
  if (!cart_is_gbmc())
  {
    Serial.print("-ERR cart is not GBMC\n");
    return;
  }

  byte map[128];
  get_gbmc_mapping(map);

  Serial.print("+OK Start dumping GBMC mapping\n");

  for (int i = 0; i < 128; i++)
  {
    Serial.write(map[i]);
  }
}

void write_gbmc_rom()
{
  if (!cart_is_gbmc())
  {
    Serial.print("-ERR cart is not GBMC\n");
    return;
  }

  cart_gbmc_enable_np_registers();
  cart_gbmc_disable_write_protect();
  cart_gbmc_map_entire();

  // unlock sector 0
  io_write_byte(0x2100, 0x01);
  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0x60);
  cart_gbmc_send_flash_cmd(0x40);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("L");
  }

  // keep mbc registers disabled for cart_gbmc_send_flash_cmd

  // erase flash
  cart_gbmc_send_flash_cmd(0x80);
  cart_gbmc_send_flash_cmd(0x10);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("E");
  }

  // reset flash
  cart_gbmc_send_flash_cmd(0xf0);

  cart_gbmc_enable_mbc_registers();
  cart_gbmc_disable_np_registers();

  Serial.print("\n");
  Serial.print("+OK give me data for whole ROM\n");

  const int flash_buf_size = 128;
  byte buf[flash_buf_size];

  unsigned int base_addr = 0x0000;
  for (unsigned int bank = 0; bank < 64; bank++)
  {
    if (bank >= 1)
    {
      base_addr = 0x4000;
    }

    cart_gbmc_enable_np_registers();
    cart_gbmc_enable_mbc_registers();
    io_write_byte(0x2100, bank);
    cart_gbmc_disable_mbc_registers();
    cart_gbmc_disable_np_registers();

    // blank check of current bank
/*
    for (unsigned int addr = 0; addr < 0x4000; addr++)
    {
      byte b = io_read_byte(base_addr + addr);
      if (0xff != b)
      {
        Serial.print("\n");
        Serial.print("-ERR Blank-Check failed at address 0x");
        Serial.print(base_addr + addr, HEX);
        Serial.print(", byte=0x");
        Serial.print(b, HEX);
        Serial.print("\n");
        return;
      }
    }
*/

    for (unsigned int addr = 0; addr < 0x4000; addr += flash_buf_size)
    {
      // read 128 bytes from serial
      int buf_count = 0;
      while (buf_count < flash_buf_size)
      {
        while (Serial.available() <= 0)
        {
          delay(1);
        }

        int value = Serial.read();
        if (value >= 0)
        {
          buf[buf_count] = (byte)value;
          buf_count++;
        }

        if (buf_count >= flash_buf_size)
        {
          break;
        }
      }

      // write flash buffer
      cart_gbmc_enable_np_registers();
      cart_gbmc_enable_mbc_registers();
      io_write_byte(0x2100, 0x01);
      cart_gbmc_disable_mbc_registers();
      cart_gbmc_disable_np_registers();

      cart_gbmc_send_flash_cmd(0xa0);
      while ((io_read_byte(0x0000) & 0x80) != 0x80)
      {
        io_write_byte(0x0000, 0x03);
        Serial.print("R");
      }

      // write 128 bytes to flash buffer
      cart_gbmc_enable_np_registers();
      cart_gbmc_enable_mbc_registers();
      io_write_byte(0x2100, bank);
      cart_gbmc_disable_mbc_registers();
      cart_gbmc_disable_np_registers();

      for (int i = 0; i < flash_buf_size; i++)
      {
        io_write_byte(base_addr + addr + i, buf[i]);
      }

      // execute write and wait for completion
      io_write_byte(base_addr + addr + flash_buf_size - 1, 0xff);
      while ((io_read_byte(base_addr + addr) & 0x80) != 0x80)
      {
        io_write_byte(0x0000, 0x03);
        Serial.print("W");
      }

      Serial.print("\n");
      Serial.print("+OK Wrote ");
      Serial.print(buf_count, DEC);
      Serial.print(" bytes in bank ");
      Serial.print(bank, DEC);
      Serial.print("/63\n");
    }

    //Serial.print("\n");
    //Serial.print("+OK Wrote bank ");
    //Serial.print(bank, DEC);
    //Serial.print("/63\n");
  }

  // reset flash
  cart_gbmc_enable_np_registers();
  cart_gbmc_enable_mbc_registers();
  io_write_byte(0x2100, 0x01);

  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0xf0);
  cart_gbmc_enable_mbc_registers();

  cart_gbmc_map_game_without_reset(0);
  cart_gbmc_disable_np_registers();
}

void write_gbmc_mapping()
{
  if (!cart_is_gbmc())
  {
    Serial.print("-ERR cart is not GBMC\n");
    return;
  }

  cart_gbmc_enable_np_registers();
  cart_gbmc_disable_write_protect();
  cart_gbmc_map_entire();

  // unlock sector 0
  io_write_byte(0x2100, 0x01);
  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0x60);
  cart_gbmc_send_flash_cmd(0x40);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("L");
  }

  // keep mbc registers disabled for cart_gbmc_send_flash_cmd

  // erase flash
  cart_gbmc_send_flash_cmd(0x60);
  cart_gbmc_send_flash_cmd(0x04);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("E");
  }

  // unlock write to map area
  cart_gbmc_send_flash_cmd(0x60);
  cart_gbmc_send_flash_cmd(0xe0);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("M");
  }
  cart_gbmc_enable_mbc_registers();
  cart_gbmc_disable_np_registers();

  Serial.print("\n");
  Serial.print("+OK give me data for mapping\n");

  // read mapping data from serial
  const int mapping_size = 128;
  byte buf[mapping_size];

  int buf_count = 0;
  while (buf_count < mapping_size)
  {
    while (Serial.available() <= 0)
    {
      delay(1);
    }

    int value = Serial.read();
    if (value >= 0)
    {
      buf[buf_count] = (byte)value;
      buf_count++;
    }

    if (buf_count >= mapping_size)
    {
      break;
    }
  }

  // write flash buffer
  cart_gbmc_send_flash_cmd(0xa0);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("R");
  }

  // write 128 bytes to flash buffer

  cart_gbmc_enable_np_registers();
  cart_gbmc_enable_mbc_registers();
  io_write_byte(0x2100, 0x00);
  cart_gbmc_disable_mbc_registers();
  cart_gbmc_disable_np_registers();

  for (int i = 0; i < mapping_size; i++)
  {
    io_write_byte(i, buf[i]);
  }

  // execute write
  io_write_byte(mapping_size - 1, 0xff);
  while ((io_read_byte(0x0000) & 0x80) != 0x80)
  {
    io_write_byte(0x0000, 0x03);
    Serial.print("W");
  }

  Serial.print("\n");
  Serial.print("+OK Wrote ");
  Serial.print(buf_count, DEC);
  Serial.print(" bytes\n");

  // reset flash
  cart_gbmc_enable_np_registers();
  cart_gbmc_enable_mbc_registers();
  io_write_byte(0x2100, 0x01);

  cart_gbmc_disable_mbc_registers();
  cart_gbmc_send_flash_cmd(0xf0);
  cart_gbmc_enable_mbc_registers();

  cart_gbmc_map_game_without_reset(0);
  cart_gbmc_disable_np_registers();
}

void dump_gbmc_titles()
{
  if (!cart_is_gbmc())
  {
    Serial.print("-ERR cart is not GBMC\n");
    return;
  }

  byte map[128];
  get_gbmc_mapping(map);

  cart_gbmc_enable_np_registers();
  cart_gbmc_map_game_without_reset(0);
  cart_gbmc_disable_np_registers();

  for (int i = 0; i < 86; i++)
  {
    if (map[i + 0x18] != 0xff)
    {
      Serial.print("-ERR cart is not GBMC MULTI CARTRIDGE\n");
      return;
    }
  }

  Serial.print("+OK start title list\n");

  cart_gbmc_enable_np_registers();
  cart_gbmc_map_entire();

  for (int n = 1; n <= 8; n++)
  {
    if (map[n * 3] == 0xff)
    {
      continue;
    }

    // get rom start bank (16kB * offset * 2)
    byte bank = (map[n * 3 + 1] & 0x7f) * 2;

    // switch bank
    io_write_byte(0x2100, bank);

    // get title
    byte title_raw[16];
    char title[17];
    for (int i = 0; i < 16; i++)
    {
      title_raw[i] = io_read_byte(0x4134 + i);
      title[i] = (0x7f & title_raw[i]);
    }
    title[16] = '\0';

    if (title_raw[15] >= 0x80)
    {
      if (title_raw[14] != 0x00)
      {
        title[11] = '\0';
      }
      else
      {
        title[15] = '\0';
      }
    }

    Serial.print(n, DEC);
    Serial.print(":[");
    Serial.print(title);
    Serial.print("]\n");
  }

  cart_gbmc_map_game_without_reset(0);
  cart_gbmc_disable_np_registers();
}

boolean select_title(int title)
{
  if (title > 0 && !cart_is_gbmc())
  {
    return false;
  }

  if (title < 0 || title > 7)
  {
    return false;
  }

  cart_gbmc_enable_np_registers();
  cart_gbmc_map_game_without_reset(title);
  delay(100);
  cart_gbmc_disable_np_registers();

  return true;
}