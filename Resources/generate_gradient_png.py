
import zlib
import struct
import math

def make_png(width, height, pixels):
    # pixels is list of (r,g,b) tuples
    # PNG requires big-endian
    
    # helper to pack chunk
    def pack_chunk(tag, data):
        to_check = tag + data
        crc = zlib.crc32(to_check) & 0xffffffff
        return struct.pack('!I', len(data)) + to_check + struct.pack('!I', crc)

    # Signature
    png_sig = b'\x89PNG\r\n\x1a\n'
    
    # IHDR: Width, Height, BitDepth(8), ColorType(2=RGB), Compression(0), Filter(0), Interlace(0)
    ihdr_data = struct.pack('!IIBBBBB', width, height, 8, 2, 0, 0, 0)
    ihdr = pack_chunk(b'IHDR', ihdr_data)
    
    # IDAT
    # Scanlines: Filter byte (0) + RGB bytes
    raw_data = b''
    for y in range(height):
        raw_data += b'\x00' # Filter type 0 (None)
        row_pixels = pixels[y * width : (y + 1) * width]
        for r, g, b in row_pixels:
            raw_data += struct.pack('BBB', r, g, b)
            
    compressed = zlib.compress(raw_data)
    idat = pack_chunk(b'IDAT', compressed)
    
    # IEND
    iend = pack_chunk(b'IEND', b'')
    
    return png_sig + ihdr + idat + iend

# Generate Gradient Data
width = 256
height = 256
c1 = (34, 34, 34) # #222222
c2 = (26, 26, 26) # #1a1a1a

pixel_data = []

# 135 degree gradient logic
# x,y origin top-left.
for y in range(height):
    for x in range(width):
        proj = (x + y) / (width + height)
        
        # Clamp proj
        proj = max(0.0, min(1.0, proj))
        
        r = int(c1[0] + (c2[0] - c1[0]) * proj)
        g = int(c1[1] + (c2[1] - c1[1]) * proj)
        b = int(c1[2] + (c2[2] - c1[2]) * proj)
        
        pixel_data.append((r, g, b))

# Write File
with open('CardGradient.png', 'wb') as f:
    f.write(make_png(width, height, pixel_data))

print("Generated CardGradient.png (Valid PNG)")
