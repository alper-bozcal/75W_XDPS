import minimalmodbus

def create_modbus_instrument(port, address=1):
    """
    Creates a Modbus instrument object
    
    Args:
        port (str): Serial port (e.g., 'COM1', 'COM58', '/dev/ttyUSB0')
        address (int): Modbus device address (default: 1)
    
    Returns:
        minimalmodbus.Instrument: Modbus instrument object
    """
    try:
        modbus_obj = minimalmodbus.Instrument(port, address)
        return modbus_obj
    except Exception as e:
        print(f"Error creating Modbus instrument: {e}")
        return None

def writeRegister(register_num, value, modbus_obj, function_code=6):
    """
    Writes a value to a Modbus register
    
    Args:
        register_num (int): Register address
        value (int): Value to write (0-65535)
        modbus_obj: Modbus instrument object
        function_code (int): Modbus function code (6=single register, 16=multiple registers)
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        if value > 65535:
            print(f"Value {value} is out of range (max 65535)")
            return False
            
        # Farklı function code'ları dene
        if function_code == 6:
            # Function Code 6: Write Single Holding Register
            modbus_obj.write_register(register_num, value, 0, 6)
        elif function_code == 16:
            # Function Code 16: Write Multiple Holding Registers
            modbus_obj.write_registers(register_num, [value])
        else:
            print(f"Unsupported function code: {function_code}")
            return False
            
        #print(f"✓ Register {register_num} = {value} (FC:{function_code})")
        return True
        
    except minimalmodbus.NoResponseError:
        print(f"No response from device for register {register_num} (FC:{function_code})")
        return False
    except minimalmodbus.IllegalRequestError:
        print(f"Illegal request for register {register_num} (FC:{function_code})")
        return False
    except minimalmodbus.ModbusException as e:
        print(f"Modbus exception for register {register_num}: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error for register {register_num} (FC:{function_code}): {e}")
        return False

def writeRegisterWithRetry(register_num, value, modbus_obj, max_retries=3):
    """
    Writes a value to a Modbus register with retry and multiple function codes
    
    Args:
        register_num (int): Register address
        value (int): Value to write (0-65535)
        modbus_obj: Modbus instrument object
        max_retries (int): Maximum number of retries
    
    Returns:
        bool: True if successful, False otherwise
    """
    function_codes = [6, 16]  # Try both function codes
    
    for fc in function_codes:
        #print(f"Trying Function Code {fc} for register {register_num}...")
        
        for attempt in range(max_retries):
            if writeRegister(register_num, value, modbus_obj, fc):
                return True
            else:
                if attempt < max_retries - 1:
                    print(f"Attempt {attempt + 1}/{max_retries} failed, retrying...")
                    import time
                    time.sleep(0.1)
                    
    print(f"All attempts failed for register {register_num}")
    return False

def writeRegisterAdvanced(register_num, value, modbus_obj, signed=False, byte_order=0):
    """
    Advanced write function with different data formats
    
    Args:
        register_num (int): Register address
        value (int): Value to write
        modbus_obj: Modbus instrument object
        signed (bool): Whether to treat value as signed
        byte_order (int): Byte order (0=big endian, 1=little endian)
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        # Convert value if signed
        if signed and value < 0:
            value = value & 0xFFFF  # Convert to unsigned 16-bit
            
        # Try different methods
        methods = [
            lambda: modbus_obj.write_register(register_num, value, 0, 6),
            lambda: modbus_obj.write_register(register_num, value, 0, 6, signed=signed),
            lambda: modbus_obj.write_registers(register_num, [value]),
        ]
        
        for i, method in enumerate(methods):
            try:
                method()
                #print(f"✓ Register {register_num} = {value} (method {i+1})")
                return True
            except Exception as e:
                print(f"Method {i+1} failed: {e}")
                continue
                
        return False
        
    except Exception as e:
        print(f"Advanced write failed for register {register_num}: {e}")
        return False

def readRegister(register_num, modbus_obj):
    """
    Reads a value from a Modbus register
    
    Args:
        register_num (int): Register address
        modbus_obj: Modbus instrument object
    
    Returns:
        int or None: Register value if successful, None otherwise
    """
    try:
        value = modbus_obj.read_register(register_num, 0)  # Register address and number of decimals
        return value
    except minimalmodbus.NoResponseError:
        print(f"No response from device for register {register_num}")
        return None
    except minimalmodbus.IllegalRequestError:
        print(f"Illegal request for register {register_num}")
        return None
    except minimalmodbus.ModbusException as e:
        print(f"Modbus exception for register {register_num}: {e}")
        return None
    except Exception as e:
        print(f"Unexpected error for register {register_num}: {e}")
        return None

def writeMultipleRegisters(start_register, values, modbus_obj):
    """
    Writes multiple values to consecutive Modbus registers
    
    Args:
        start_register (int): Starting register address
        values (list): List of values to write
        modbus_obj: Modbus instrument object
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        modbus_obj.write_registers(start_register, values)
        return True
    except minimalmodbus.NoResponseError:
        print(f"No response from device for registers starting at {start_register}")
        return False
    except minimalmodbus.IllegalRequestError:
        print(f"Illegal request for registers starting at {start_register}")
        return False
    except minimalmodbus.ModbusException as e:
        print(f"Modbus exception for registers starting at {start_register}: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error for registers starting at {start_register}: {e}")
        return False

def readMultipleRegisters(start_register, count, modbus_obj):
    """
    Reads multiple values from consecutive Modbus registers
    
    Args:
        start_register (int): Starting register address
        count (int): Number of registers to read
        modbus_obj: Modbus instrument object
    
    Returns:
        list or None: List of register values if successful, None otherwise
    """
    try:
        values = modbus_obj.read_registers(start_register, count)
        return values
    except minimalmodbus.NoResponseError:
        print(f"No response from device for registers starting at {start_register}")
        return None
    except minimalmodbus.IllegalRequestError:
        print(f"Illegal request for registers starting at {start_register}")
        return None
    except minimalmodbus.ModbusException as e:
        print(f"Modbus exception for registers starting at {start_register}: {e}")
        return None
    except Exception as e:
        print(f"Unexpected error for registers starting at {start_register}: {e}")
        return None

def testConnection(modbus_obj, test_register=1):
    """
    Tests the Modbus connection by trying to read a register
    
    Args:
        modbus_obj: Modbus instrument object
        test_register (int): Register to test (default: 1)
    
    Returns:
        bool: True if connection is working, False otherwise
    """
    try:
        value = modbus_obj.read_register(test_register, 0)
        print(f"Connection test successful. Register {test_register} value: {value}")
        return True
    except Exception as e:
        print(f"Connection test failed: {e}")
        return False

# Backward compatibility - Default connection (can be overridden)
DEFAULT_PORT = 'COM1'
DEFAULT_ADDRESS = 1

# Create default modbus object for backward compatibility
modbus_obj = create_modbus_instrument(DEFAULT_PORT, DEFAULT_ADDRESS)

# Usage examples
if __name__ == "__main__":
    print("=== MODBUS FUNCTION TEST ===")
    
    # Test with default connection
    print(f"\n1. Testing default connection (Port: {DEFAULT_PORT}, Address: {DEFAULT_ADDRESS})")
    if modbus_obj:
        if testConnection(modbus_obj):
            print("✓ Default connection is working")
            
            # Test read/write
            test_reg = 100
            test_value = 1234
            
            print(f"\n2. Testing write operation (Register {test_reg}, Value {test_value})")
            if writeRegister(test_reg, test_value, modbus_obj):
                print("✓ Write successful")
                
                print(f"\n3. Testing read operation (Register {test_reg})")
                read_value = readRegister(test_reg, modbus_obj)
                if read_value is not None:
                    print(f"✓ Read successful: {read_value}")
                    if read_value == test_value:
                        print("✓ Read/Write test passed!")
                    else:
                        print(f"✗ Value mismatch: wrote {test_value}, read {read_value}")
                else:
                    print("✗ Read failed")
            else:
                print("✗ Write failed")
        else:
            print("✗ Default connection failed")
    else:
        print("✗ Could not create default connection")
    
    # Test with custom connection
    print(f"\n4. Testing custom connection")
    custom_port = input("Enter custom port (or press Enter to skip): ").strip()
    if custom_port:
        custom_address = int(input("Enter device address (default 1): ") or "1")
        custom_modbus = create_modbus_instrument(custom_port, custom_address)
        
        if custom_modbus:
            if testConnection(custom_modbus):
                print(f"✓ Custom connection working (Port: {custom_port}, Address: {custom_address})")
            else:
                print(f"✗ Custom connection failed (Port: {custom_port}, Address: {custom_address})")
        else:
            print(f"✗ Could not create custom connection")
    else:
        print("Skipping custom connection test")
    
    print("\nTest completed!")