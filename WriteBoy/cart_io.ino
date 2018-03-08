/*
 * GB Cartridge low-level I/O
 * 
 * Designed for Arduino Mega 2560 R3 with following wiring:
 * 
 * | GB cart | Arduino Mega 2560 |
 * |--------:|:------------------|
 * |  1  VCC | VCC               |
 * |  2  CLK | GND               |
 * |  3  /WR | PG2 39  -------   |
 * |  4  /RD | PG1 40   PORTG    |
 * |  5  /CS | PG0 41  -------   |
 * |  6   A0 | PA0 22  -------   |
 * |  7   A1 | PA1 23     :      |
 * |  8   A2 | PA2 24     :      |
 * |  9   A3 | PA3 25   PORTA    |
 * | 10   A4 | PA4 26  Addr Lo   |
 * | 11   A5 | PA5 27     :      |
 * | 12   A6 | PA6 28     :      |
 * | 13   A7 | PA7 29  -------   |
 * | 14   A8 | PC0 37  -------   |
 * | 15   A9 | PC1 36     :      |
 * | 16  A10 | PC2 35     :      |
 * | 17  A11 | PC3 34   PORTC    |
 * | 18  A12 | PC4 33  Addr Hi   |
 * | 19  A13 | PC5 32     :      |
 * | 20  A14 | PC6 31     :      |
 * | 21  A15 | PC7 30  -------   |
 * | 22   D0 | PL0 49  -------   |
 * | 23   D1 | PL1 48     :      |
 * | 24   D2 | PL2 47     :      |
 * | 25   D3 | PL3 46   PORTL    |
 * | 26   D4 | PL4 45   Data     |
 * | 27   D5 | PL5 44     :      |
 * | 28   D6 | PL6 43     :      |
 * | 29   D7 | PL7 42  -------   |
 * | 30 /RST | VCC (via 10kohm)  |
 * | 31  AIN | NC  (unused)      |
 * | 32  GND | GND               |
 */

#define ADDR_LO_DDR DDRA
#define ADDR_LO_OUT PORTA

#define ADDR_HI_DDR DDRC
#define ADDR_HI_OUT PORTC

#define DATA_DDR DDRL
#define DATA_IN  PINL
#define DATA_OUT PORTL

#define CTRL_DDR DDRG
#define CTRL_OUT PORTG

#define WR PG2
#define RD PG1
#define CS PG0
#define io_clear(pin)  CTRL_OUT |=  (1 << pin);  // set HIGH to specified pin
#define io_set(pin)    CTRL_OUT &= ~(1 << pin);  // set LOW to specified pin

void io_setup()
{
  ADDR_LO_DDR = 0xff; // lower 8bit of address: output
  ADDR_HI_DDR = 0xff; // upper 8bit of address: output
  CTRL_DDR |= ((1 << WR) | (1 << RD) | (1 << CS)); // /WR, /RD, /CS: output
  DATA_DDR = 0x00; // data: input

  io_clear(WR);
  io_set(RD);
  io_clear(CS);
}

byte io_read_byte(unsigned int address)
{
  byte value;

  // set address
  ADDR_LO_OUT = address & 0xff;
  ADDR_HI_OUT = (address >> 8) & 0xff;

  // on read from SRAM, activate /CS
  if ((address & 0x8000) == 0x8000) { io_set(CS); }

  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  // read a byte
  value = DATA_IN;

  if ((address & 0x8000) == 0x8000) { io_clear(CS); }

  return value;
}

void io_write_byte(unsigned int address, byte value)
{
  io_clear(RD);

  // set address
  ADDR_LO_OUT = address & 0xff;
  ADDR_HI_OUT = (address >> 8) & 0xff;

  // output data
  DATA_DDR = 0xff;  // set data bus as output
  DATA_OUT = value;
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  // write a byte
  // on write to SRAM, activate /CS
  if ((address & 0x8000) == 0x8000) { io_set(CS); }
  io_set(WR);
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  // placing less than 4 NOPs seems to be unstable on writing to FlashROM, as I tested

  io_clear(WR);
  if ((address & 0x8000) == 0x8000) { io_clear(CS); }
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  DATA_DDR = 0x00;  // set data bus as input

  io_set(RD);
}
