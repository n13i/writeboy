# WriteBoy

WriteBoy is an Arduino based ROM image writer for GB Memory Cartridge. 

## Requirement

### Software
- Arduino IDE
- Python 3.x and pyserial

### Hardware
- Arduino Mega 2560 R3
- GB Memory Cartridge (DMG-MMSA-JPN)

### Interface
![schematic](https://raw.githubusercontent.com/n13i/writeboy/master/WriteBoy/schematic.png)

## Usage

### Writing to GBMC
Assume that Arduino is connected to COM3.

1. Prepare a ROM image (.gb) and MAP file (.map) - using [GB Memory Binary Maker](https://github.com/Infinest/GB-Memory-Binary-Maker) would be nice
2. Write a MAP to GBMC by followng command:
```python writeboy.py --port COM3 --write-gbmc-mapping filename.map```
3. Write a ROM image to GBMC by followng command:
```python writeboy.py --port COM3 --write-gbmc-rom filename.gb```

### Other features
- SRAM Read/write
- ROM dump

See ```python writeboy.py --help```.

## References
- [Pan Docs \- GbdevWiki](http://gbdev.gg8.se/wiki/articles/Pan_Docs) - for about GB cartridge header and Memory Bank Controllers
- [nesdev\.com • View topic \- How to program a NINTENDO POWER Cartridge ?](https://forums.nesdev.com/viewtopic.php?f=12&t=11453) - for informations about GB Memory Cartridge specific registers and mapping table structure
- [GBCartRead – Gameboy Cart Reader « insideGadgets](https://www.insidegadgets.com/projects/gbcartread-gameboy-cart-reader/)
- [sanni/cartreader: A shield for the Arduino Mega that can read video game cartridges\.](https://github.com/sanni/cartreader)
