#!/usr/bin/python3
# -*- coding: utf-8 -*-

import serial
import sys
import argparse

logo = bytes([0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e])

cart_types = {
    0x00: 'ROM ONLY',
    0x01: 'ROM+MBC1',
    0x02: 'ROM+MBC1+RAM',
    0x03: 'ROM+MBC1+RAM+BATT',
    0x05: 'ROM+MBC2',
    0x06: 'ROM+MBC2+BATTERY',
    0x08: 'ROM+RAM',
    0x09: 'ROM+RAM+BATTERY',
    0x0b: 'ROM+MMM01',
    0x0c: 'ROM+MMM01+SRAM',
    0x0d: 'ROM+MMM01+SRAM+BATT',
    0x0f: 'ROM+MBC3+TIMER+BATT',
    0x10: 'ROM+MBC3+TIMER+RAM+BATT',
    0x11: 'ROM+MBC3',
    0x12: 'ROM+MBC3+RAM',
    0x13: 'ROM+MBC3+RAM+BATT',
    0x19: 'ROM+MBC5',
    0x1a: 'ROM+MBC5+RAM',
    0x1b: 'ROM+MBC5+RAM+BATT',
    0x1c: 'ROM+MBC5+RUMBLE',
    0x1d: 'ROM+MBC5+RUMBLE+SRAM',
    0x1e: 'ROM+MBC5+RUMBLE+SRAM+BATT',
    0x20: 'ROM+MBC6',
    0x22: 'ROM+MBC7+SENSOR+RUMBLE+RAM+BATTERY',
    0xfc: 'Pocket Camera',
    0xfd: 'Bandai TAMA5',
    0xfe: 'Hudson HuC-3',
    0xff: 'Hudson HuC-1',
}

rom_sizes = {
    0x00: '32 kbytes',
    0x01: '64 kbytes',
    0x02: '128 kbytes',
    0x03: '256 kbytes',
    0x04: '512 kbytes',
    0x05: '1 Mbytes',
    0x06: '2 Mbytes',
    0x07: '4 Mbytes',
    0x08: '8 Mbytes',
    0x52: '1.152 Mbytes',
    0x53: '1.28 Mbytes',
    0x54: '1.536 Mbytes',
}

ram_sizes = {
    0x00: 'None',
    0x01: '2 kbytes',
    0x02: '8 kbytes',
    0x03: '32 kbytes',
    0x04: '128 kbytes',
    0x05: '64 kbytes',
}

rotor = ['\\', '|', '/', '-']

def get_info(serial):
    serial.readline()
    send_request(serial, 'HEADER')
    receive_response(serial)

    # read rom 0x0100-0x014f
    header = bytes()
    while 1:
        buf = serial.read(80)
        if len(buf) == 0:
            break
        header += buf

    #print('{}'.format(''.join([r'0x{:02x} '.format(x) for x in header])))

    is_logo_ok = (logo == header[0x04:0x34])

    title_len = 16
    if header[0x43] >= 0x80:
        # has CGB flag, length of title is 11 or 15
        if header[0x42] != 0x00:
            # if 0x42 is not 0x00, maybe 0x3f-0x42 are used as manufacturer code, not as title
            title_len = 11
        else:
            title_len = 15
        
    ng_chars = b'"*/:<>?\\'
    title = bytearray(header[0x34:(0x34 + title_len)])
    for i, c in enumerate(title):
        if c == 0x00:
            title[i] = 0x20
        elif c < 0x20 or c > 0x7a:
            title[i] = 0x5f
        try:
            ng_chars.index(c)
            title[i] = 0x5f
        except ValueError:
            pass

    info = {}
    info['title'] = title.decode('utf-8').rstrip()
    info['logo_check_ok'] = is_logo_ok
    info['cart_type'] = cart_types.get(header[0x47], 'Unknown')
    info['rom_size'] = rom_sizes.get(header[0x48], 'Unknown')
    info['ram_size'] = ram_sizes.get(header[0x49], 'Unknown')

    return info

def show_info(info):
    print('Logo check: {}'.format('OK' if info['logo_check_ok'] else 'NG'))
    print('Title: [{}]'.format(info['title']))
    print('Cartridge type: {}'.format(info['cart_type']))
    print('{0} ROM / {1} RAM'.format(info['rom_size'], info['ram_size']))
    print('')

    return

def check_logo(info, skip_check=False):
    if not info['logo_check_ok'] and not skip_check:
        sys.exit()
    return

def get_filename(info, ext, spec=None):
    if spec == None:
        return info['title'] + '.' + ext

    return spec

def dump_rom(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'DUMP ROM')
    receive_response(ser, args.verbose)

    receive_image(ser, get_filename(info, 'gb', args.filename))
    return

def dump_gbmc_rom(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'DUMP GBMCROM')
    receive_response(ser, args.verbose)

    receive_image(ser, get_filename(info, 'gb', args.filename))
    return

def dump_sram(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    if args.title != None:
        send_request(ser, 'DUMP SRAM ' + args.title)
    else:
        send_request(ser, 'DUMP SRAM')

    receive_response(ser, args.verbose)

    receive_image(ser, get_filename(info, 'sav', args.filename))
    return

def dump_mapping(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'DUMP MAPPING')
    receive_response(ser, args.verbose)

    receive_image(ser, get_filename(info, 'map', args.filename))
    return

def dump_gbmc_titles(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'DUMP TITLES')
    receive_response(ser, args.verbose)

    while 1:
        buf = ser.readline()
        if len(buf) == 0:
            break
        print(buf.decode(), end='')

    return

def write_sram(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    if args.title != None:
        send_request(ser, 'WRITE SRAM ' + args.title)
    else:
        send_request(ser, 'WRITE SRAM')

    receive_response(ser, args.verbose)

    send_image(ser, get_filename(info, 'sav', args.filename))
    return

def write_gbmc_rom(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'WRITE GBMCROM')
    receive_response(ser, args.verbose)

    send_image(ser, get_filename(info, 'gb', args.filename))
    return

def write_mapping(args, ser):
    info = get_info(ser)
    show_info(info)
    check_logo(info, skip_check=args.skip_check)

    send_request(ser, 'WRITE MAPPING')
    receive_response(ser, args.verbose)

    send_image(ser, get_filename(info, 'map', args.filename))
    return

def print_info(args, ser):
    info = get_info(ser)
    show_info(info)

    return


def send_request(ser, request):
    print("<< {}".format(request))
    ser.write((request + '\n').encode())

def receive_response(ser, verbose=False, nonstop_on_error=False):
    while 1:
        response = (ser.readline()).decode()
        if response.startswith('-'):
            print("\n>> {}".format(response), end='')
            if not nonstop_on_error:
                sys.exit()
            else:
                break
        elif response.startswith('+'):
            if verbose:
                print("\n>> {}".format(response), end='')
            break
        else:
            if verbose:
                print("\n** {}".format(response), end='')


def receive_image(ser, filename):
    print('Target file: {0}'.format(filename))
    f = open(filename, 'wb')
    count = 0
    byte = 0
    while 1:
        buf = ser.read(256)
        if len(buf) == 0:
            break
        f.write(buf)
        byte += len(buf)
        count += 1
        print('\r {0} Received {1:4d} kbytes ({2:7d} bytes)'.format(rotor[count % 4], int(byte/1024), byte), end='')
        sys.stdout.flush()
    print('\rOK Received {0:4d} kbytes ({1:7d} bytes)'.format(int(byte/1024), byte))
    f.close()
    return

def send_image(ser, filename, verbose=False):
    print('Target file: {0}'.format(filename))
    f = open(filename, 'rb')
    count = 0
    byte = 0
    while 1:
        buf = f.read(128)
        if not buf:
            break
        ser.write(buf)
        receive_response(ser, verbose)
        byte += len(buf)
        count += 1
        print('\r {0} Sent {1:4d} kbytes ({2:7d} bytes)'.format(rotor[count % 4], int(byte/1024), byte), end='')
        sys.stdout.flush()
    print('\rOK Sent {0:4d} kbytes ({1:7d} bytes)'.format(int(byte/1024), byte))
    f.close()
    return


parser = argparse.ArgumentParser(description='Reads ROM/SRAM and writes SRAM from/to a GB cartridge. Also writes ROM to a GB Memory Cartridge.')
parser.add_argument('--port',
                    nargs='?', default=None, required=True,
                    help='serial port connected to Arduino (ex. COM3; required)')
parser.add_argument('--baudrate',
                    nargs='?', default=115200,
                    help='serial port baudrate (default: 115200)')
parser.add_argument('--info',
                    dest='action', action='store_const', const=print_info,
                    help='show cartridge informations from header and exit')
parser.add_argument('--dump-rom',
                    dest='action', action='store_const', const=dump_rom,
                    help='dump ROM (on GBMC, dumps first block only)')
parser.add_argument('--dump-sram',
                    dest='action', action='store_const', const=dump_sram,
                    help='dump SRAM')
parser.add_argument('--dump-gbmc-rom',
                    dest='action', action='store_const', const=dump_gbmc_rom,
                    help='dump all blocks from GBMC rom as single block')
parser.add_argument('--dump-gbmc-mapping',
                    dest='action', action='store_const', const=dump_mapping,
                    help='dump mapping from GBMC')
parser.add_argument('--dump-gbmc-titles',
                    dest='action', action='store_const', const=dump_gbmc_titles,
                    help='dump titles in GBMC MULTI CARTRIDGE')
parser.add_argument('--write-sram',
                    dest='action', action='store_const', const=write_sram,
                    help='write SRAM')
parser.add_argument('--title',
                    nargs='?', default=None,
                    help='specify the title number (1..7) of desired to read/write SRAM in GBMC MULTI CARTRIDGE')
parser.add_argument('--write-gbmc-rom',
                    dest='action', action='store_const', const=write_gbmc_rom,
                    help='write ROM to GBMC')
parser.add_argument('--write-gbmc-mapping',
                    dest='action', action='store_const', const=write_mapping,
                    help='write mapping to GBMC')
parser.add_argument('--skip-check',
                    dest='skip_check', action='store_true',
                    help='skip header check (may produce corrupted rom image)')
parser.add_argument('--verbose',
                    dest='verbose', action='store_true',
                    help='show verbose log')
parser.add_argument('filename',
                    nargs='?', default=None,
                    help='specify a file name to read/write (or determined by title from cartridge header)')

args = parser.parse_args()
if args.action == None:
    parser.print_usage()
    sys.exit()

ser = serial.Serial(port=args.port, baudrate=args.baudrate, timeout=1)
args.action(args, ser)
ser.close()
