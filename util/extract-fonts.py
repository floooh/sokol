'''
    Helper script to extract font data from home computer ROM images,
    print C source to stdout.

    Expects the following ROM image files in the current directory:

    - caos31.853
    - caos42e.854
    - z1013_font.bin
    - cpc6128_os.bin
    - c64_char.bin
    - oric_atmos_v122.rom

    NOTE: run with python3
'''

# extract a byte range from a ROM image file and return an array of bytes
def extract_bytes(filename, offset, num_bytes):
    with open(filename, 'rb') as f:
        return f.read()[offset: offset + num_bytes ]

def invert(font_data):
    return [(~i & 0xFF) for i in font_data]

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
    out_data[0x80*8 : 0x100*8] = invert(out_data[0x00*8 : 0x80*8])
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
    out_data[0x80*8 : 0x100*8] = invert(out_data[0x00*8 : 0x80*8])
    return out_data

def extract_z1013():
    # the Z1013 has a separate linear font ROM with 256 chars
    return extract_bytes('z1013_font.bin', 0x0000, 0x0800)

def extract_cpc():
    return extract_bytes('cpc6128_os.bin', 0x3800, 0x0800)

def extract_c64():
    rom_0 = extract_bytes('c64_char.bin', 0x0000, 0x0800)
    rom_1 = extract_bytes('c64_char.bin', 0x0800, 0x0800)
    out_data = [0]*(0x100 * 8)
    out_data[0x00*8 : 0x20*8]  = rom_1[0x60*8 : 0x80*8]
    out_data[0x20*8 : 0x40*8]  = rom_0[0x20*8 : 0x40*8]  # !@#$...12345...
    out_data[0x40*8 : 0x60*8]  = rom_0[0x00*8 : 0x20*8]  # @ABCD...
    out_data[0x60*8 : 0x80*8]  = rom_1[0x00*8 : 0x20*8]  # @abcd...
    out_data[0x60]             = rom_1[0x40]
    out_data[0x80*8 : 0xC0*8]  = rom_0[0x40*8 : 0x80*8]
    out_data[0xC0*8 : 0x100*8] = rom_0[0xC0*8 : 0x100*8]
    return out_data

def extract_oric():
    font_bytes = extract_bytes('oric_atmos_v122.rom', 0x3C78, 0x0300)
    out_data = [0]*(0x100 * 8)
    out_data[0x100 : 0x400] = font_bytes[0 : 0x300]
    out_data[0x500 : 0x800] = invert(font_bytes[0 : 0x300])
    return out_data

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
print_c_array('CPC', '_sdtx_font_cpc', extract_cpc())
print_c_array('C64', '_sdtx_font_c64', extract_c64())
print_c_array('ORIC', '_sdtx_font_oric', extract_oric())
