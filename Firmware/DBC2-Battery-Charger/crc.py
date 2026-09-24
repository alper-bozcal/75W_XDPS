import os

def crc32(msg):
    crc = 0xf8c9140a
    for b in msg:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA0000001 if crc & 1 else crc >> 1
    return crc ^ 0xf8c9140a
"""
if __name__ == "__main__":
    fileName = "D:\\OneDrive - ENKO Elektronik\\STWS\\DBC2-Battery-Charger\\Firmware\\DBC2-Battery-Charger\\Debug\\DBC2-Battery-Charger.bin"
    with open(fileName, mode='rb') as file:
        fileContent = file.read()
        crc = hex(crc32(fileContent))
        print(crc)
        with open("deneme.hex", "rb") as hfile:
            line = (hfile.read().find(b":04F7FC00454E4B4FDC"))
            contents = hfile.readlines()
            contents.insert(line, ((":04" + "F7F8" + "00" + str(crc)[2:] + "DC")))
        with open ("deneme.hex", "wb") as hfile:
            contents = "".join(contents)
            hfile.write(bytes(contents, "ascii"))
"""