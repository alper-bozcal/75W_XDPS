try:
    import serial
except ImportError as e:
    print("Error importing a module pySerial. Exiting...")
    print(e)
    exit(1)

# Custom exceptions for Modbus errors
class CRCerror(Exception):
    pass
class ModbusError(Exception):
    pass

# Baud rate for serial port comm. Should left default for VCOM USB
DEFBAUDRATE = 115200
# Wait time for serial port comm. Should left default for normal cases
DEFCOMMTIMEOUT = 1
# Constant value for modbus protocol. It includes first three bytes and last two bytes of modbus packet.
MODBUSHEADERSIZE = 5
# Constant CRC value for modbus protocol. It includes last two bytes of modbus packet. Defaults to 2 for CRC16
MODBUSCRCSIZE = 2
# Connected devices modbus register size as bytes. If device has 32 bit addressing convention, this should change to 4. Or else should be left default.
DEVICEMBREGSIZEASBYTES = 2
# Modbus protocol constants
WRITE_HOLDING = b'\x06'
READ_HOLDING = b'\x03'

class ceyModbus:
    """Serial Port ModBus Protocol Communication and parser class. 
        It does need pySerial to be installed.
    Returns:
        _type_: ceyModbus object for serial port modbus communication
    """
    
    def __init__(self, PORT: str, ID: int = 1):
        """Init the modbus comm over USB comport with device

        Args:
            PORT (str): Com port name of the device connected. EX: "COM6". EX: "/dev/ttyACM0"
            ID (int, optional): Device ID if there is more than one device on the same channel.
                                Should left default for VCOM USB cases. Defaults to 1.
        """    
        try:
            # Convert ID integer to ID bytes for packet preps
            self._ID = ID.to_bytes(1, byteorder='big')
            # Connect to serial device
            self._PORT = serial.Serial(PORT, baudrate = DEFBAUDRATE, timeout = DEFCOMMTIMEOUT)
        except Exception as e:
            print("Error opening serial port")
            print(e)
            return None

    def _getCRC16(self, data: bytes) -> int:
        """This function returns CRC16 for ModBus of the incoming string as an integer.
        Polynomials are default for ModBus and can be easily found.

        Args:
            data (bytes): byte array to calculate CRC16 for.

        Returns:
            int: CRC16 value of the byte string supplied
        """
        crc = 0xFFFF
        for n in range(len(data)):
            crc ^= data[n]
            for i in range(8):
                if crc & 1:
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        return crc
    
    def _isCRC16OK(self, DATA: bytes) -> bool:
        """Returns True if params CRC16 for modbus is True this means the last two bytes of the string. Returns False if not gets right CRC

        Args:
            DATA (bytes): byte string for check

        Returns:
            bool: CRC16 situation
        """
        #Parse param byte string
        _crc = DATA[-MODBUSCRCSIZE:]
        _data = DATA[:-MODBUSCRCSIZE]
        
        #Calculate CRC16 for byte strnig without CCRC16 at the end
        _calcCRC = self._getCRC16(_data).to_bytes(2, byteorder = 'little')

        #Check if this two CRCs are the same
        return True if _crc == _calcCRC else False
    
    def write(self, ADDR: int, DATA: int) -> int:
        """write single data to param address

        Args:
            ADDR (int): Address to write
            DATA (int): data to write

        Raises:
            ValueError: Raises if not True CRC16 of the return packet

        Returns:
            int: returned data from the device itself
        """
        # Prepare and send packet to serial port
        # Convert params to bytes for preps.
        # endianness is big because of ModBus.
        ADDR = ADDR.to_bytes(DEVICEMBREGSIZEASBYTES, byteorder='big')
        DATA = DATA.to_bytes(DEVICEMBREGSIZEASBYTES, byteorder='big')
        # Prepare cmd for modbus
        cmd = self._ID + WRITE_HOLDING + ADDR + DATA
        # endianness of CRC16 is little in ModBus protocol.
        crc = self._getCRC16(cmd).to_bytes(DEVICEMBREGSIZEASBYTES, byteorder = 'little')
        cmd = cmd + crc
        self._PORT.write(cmd)
        # Read response
        response = self._PORT.read(MODBUSHEADERSIZE + 3)
        # CRC16 of the response from the device should be true for evaluation
        if not self._isCRC16OK(response):
            raise CRCerror('Wrong CRC for response packet.')
        elif response[1] != int.from_bytes(WRITE_HOLDING,"big"):
            raise ModbusError('Modbus error: ' + str(response[1]))
        # Return the data from the device
        frame = response[MODBUSHEADERSIZE - 1: -MODBUSCRCSIZE]
        int_values = [int.from_bytes(frame[i:i+2], 'big', signed = True) for i in range(0, len(frame), 2)]
        return int_values#int.from_bytes(response[MODBUSHEADERSIZE - MODBUSCRCSIZE: -MODBUSCRCSIZE], byteorder = "big")

    def read(self, ADDR: int, CNT: int = 1) -> int:
        """read  single data from param address

        Args:
            ADDR (int): Addr to read

        Raises:
            ValueError: Raises if not True CRC16 of the return packet

        Returns:
            int: returned data from the device itself
        """
        # prepare and send packet to serial port
        # Convert params to bytes for preps.
        # endianness is big because of ModBus.
        addr = ADDR.to_bytes(DEVICEMBREGSIZEASBYTES, byteorder='big')
        cnt = CNT.to_bytes(DEVICEMBREGSIZEASBYTES, byteorder='big')
        # Prepare cmd for modbus
        cmd = self._ID + READ_HOLDING + addr + cnt
        # endianness of CRC16 is little in ModBus protocol.
        crc = self._getCRC16(cmd).to_bytes(DEVICEMBREGSIZEASBYTES, byteorder = 'little')
        cmd = cmd + crc
        self._PORT.write(cmd)
        # read response
        response = self._PORT.read(MODBUSHEADERSIZE + (DEVICEMBREGSIZEASBYTES * CNT))
        # CRC16 of the response from the device should be true for evaluation
        if not self._isCRC16OK(response):
            raise CRCerror('Wrong CRC for response packet.')
        elif response[1] != int.from_bytes(READ_HOLDING,"big"):
            raise ModbusError('Modbus error: ' + str(response[1]))
        # Return the data from the device
        frame = response[MODBUSHEADERSIZE - MODBUSCRCSIZE: -MODBUSCRCSIZE]
        int_values = [int.from_bytes(frame[i:i+2], 'big', signed = True) for i in range(0, len(frame), 2)]
        return int_values#int.from_bytes(response[MODBUSHEADERSIZE - MODBUSCRCSIZE: -MODBUSCRCSIZE], byteorder = "big")

    def __del__(self):
        """Closes the port and exits the class"""
        try:
            self._PORT.close()
        except Exception as e:
            print("Error closing serial port")
    