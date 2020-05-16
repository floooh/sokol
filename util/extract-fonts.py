'''
    Helper script to extract font data from home computer ROM images,
    print C source to stdout.

    NOTE: run with python3
'''

# extract a byte range from a ROM image file and return an array of bytes
def extract_bytes(filename, offset, num_bytes):
    with open(filename, 'rb') as f:
        return f.read()[offset: offset + num_bytes ]

# system specific functions, return 256*8 bytes font data arrays
def extract_kc85_3():
    # KC85/3 only has 128 unique characters in ROM, repeated at 128..255

    # 0x20..0x5F
    rom_bytes_0 = extract_bytes('caos31.853', 0x0E00, 0x0200)
    # 0x00..0x1F, 0x60..0x7F,
    rom_bytes_1 = extract_bytes('caos31.853', 0x1E00, 0x0200)

    # remap the ROM bytes to a continuous array
    out_data = [0]*(0x100 * 8)
    out_data[0x00*8 : 0x20*8] = rom_bytes_1[0x00*8 : 0x20*8]
    out_data[0x20*8 : 0x60*8] = rom_bytes_0[0x00*8 : 0x40*8]
    out_data[0x60*8 : 0x80*8] = rom_bytes_1[0x20*8 : 0x40*8]
    out_data[0x80*8 : 0x100*8] = out_data[0x00*8 : 0x80*8]
    return out_data

def extract_kc85_4():
    # KC85/4 font data has the same memory layout as KC85/3, just different bitmaps

    # 0x20..0x5F
    rom_bytes_0 = extract_bytes('caos42e.854', 0x0E00, 0x0200)
    # 0x00..0x1F, 0x60..0x7F,
    rom_bytes_1 = extract_bytes('caos42e.854', 0x1E00, 0x0200)

    # remap the ROM bytes to a continuous array
    out_data = [0]*(0x100 * 8)
    out_data[0x00*8 : 0x20*8] = rom_bytes_1[0x00*8 : 0x20*8]
    out_data[0x20*8 : 0x60*8] = rom_bytes_0[0x00*8 : 0x40*8]
    out_data[0x60*8 : 0x80*8] = rom_bytes_1[0x20*8 : 0x40*8]
    out_data[0x80*8 : 0x100*8] = out_data[0x00*8 : 0x80*8]
    return out_data

def extract_z1013():
    # the Z1013 has a separate linear font ROM with 256 chars
    return extract_bytes('z1013_font.bin', 0x0000, 0x0800)

# print a C array with the output bytes
def print_c_array(system_name, array_name, bytes):
    print(f'#if defined(SOKOL_DEBUGTEXT_FONT_{system_name})')
    print(f'static const uint8_t {array_name}[2048] = {{')
    for i, byte in enumerate(bytes):
        if 0 == (i % 8):
            print('    ', end='')
        print(f'0x{byte:02X}, ', end='')
        if 0 == ((i+1) % 8):
            print(f'// {int(i/8):02X}')
    print('};')
    print('#endif')

# execution starts here
print_c_array('KC853', '_sdtx_font_kc853', extract_kc85_3())
print_c_array('KC854', '_sdtx_font_kc854', extract_kc85_4())
print_c_array('Z1013', '_sdtx_font_z1013', extract_z1013())
