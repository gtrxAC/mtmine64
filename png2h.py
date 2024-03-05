import sys
from PIL import Image
from os import path
import re

if len(sys.argv) < 2:
    print("File name not specified")
    exit(1)
files = sys.argv[1:]

with open("src/images.h", 'w') as output:
    output.write('#include "common.h"\n\n')

    for image_file in files:
        basename = path.basename(image_file).lower()
        noext = re.sub(r"\..*$", "", basename)
        nospaces = re.sub(r"\s", "_", noext)
        variable_name = re.sub(r"[^\w]", "", nospaces)

        image = Image.open(image_file)
        width, height = image.size

        if width % 4 != 0:
            print("Image width must be divisible by 4")
            exit(1)
        if width > 16:
            print("Image width is too large")
            exit(1)
        if height > 16:
            print("Image height is too large")
            exit(1)

        output.write(f"// {image_file}\n")
        output.write("Image img_" + variable_name + " = {\n")
        output.write(f"\t0b{ format(int(width/4 - 1) << 2 | int(height/4 - 1), '08b') },\n")
        output.write("\t{\n")

        data = list(image.getdata(0))

        for y in range(0, height):
            output.write("\t\t")
            for x in range(0, int(width/4)):
                byte = 0
                for i in range(3, -1, -1):
                    pixel = data[y*width + x*4 + 3 - i]
                    byte |= (round(pixel/85) << (i*2))
                output.write(f"0b{ format(byte, '08b') }, ")
            output.write("\n")
        output.write("\t}\n")
        output.write("};\n\n")