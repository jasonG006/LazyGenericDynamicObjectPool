
import struct
import math

width = 256
height = 256

# Colors
c1 = (34, 34, 34) # #222222
c2 = (26, 26, 26) # #1a1a1a

def write_bmp(filename, width, height, pixels):
    # BMP constants
    file_size = 54 + width * height * 3
    reserved = 0
    offset = 54
    header_size = 40
    planes = 1
    bpp = 24
    compression = 0
    img_size = width * height * 3
    x_ppm = 0
    y_ppm = 0
    colors_used = 0
    colors_important = 0

    with open(filename, 'wb') as f:
        # File Header (14 bytes)
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<H', reserved))
        f.write(struct.pack('<H', reserved))
        f.write(struct.pack('<I', offset))
        
        # Info Header (40 bytes)
        f.write(struct.pack('<I', header_size))
        f.write(struct.pack('<I', width))
        f.write(struct.pack('<I', height))
        f.write(struct.pack('<H', planes))
        f.write(struct.pack('<H', bpp))
        f.write(struct.pack('<I', compression))
        f.write(struct.pack('<I', img_size))
        f.write(struct.pack('<I', x_ppm))
        f.write(struct.pack('<I', y_ppm))
        f.write(struct.pack('<I', colors_used))
        f.write(struct.pack('<I', colors_important))
        
        # Pixel Data (Bottom-up)
        # BMP rows are padded to 4 bytes
        padding = (4 - (width * 3) % 4) % 4
        
        for y in range(height - 1, -1, -1):
            for x in range(width):
                 # Gradient logic
                proj = (x + y) / (width + height)
                
                r = int(c1[0] + (c2[0] - c1[0]) * proj)
                g = int(c1[1] + (c2[1] - c1[1]) * proj)
                b = int(c1[2] + (c2[2] - c1[2]) * proj)
                
                # BGR
                f.write(struct.pack('BBB', b, g, r))
            
            f.write(b'\x00' * padding)

pixels = [] # Not used in loop structure above
write_bmp('CardGradient.bmp', width, height, pixels)
print("Generated CardGradient.bmp")
