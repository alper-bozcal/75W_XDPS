from ceyModbus import ceyModbus
import time
import csv

# Create/open a CSV file for writing
csv_filename = "pwm_current_log.csv"

try:
    DEV = ceyModbus("COM38")
    DEV.write(9962, 1234)
except Exception as e:
    print("Error opening serial port")
    exit(1)

# Open the CSV file and create a writer object
with open(csv_filename, 'w', newline='') as csvfile:
    # Create a CSV writer
    csv_writer = csv.writer(csvfile)
    
    # Write the header row
    csv_writer.writerow(["PWM", "Current"])
    
    print("PWM,Current")
    
    for i in range(0, 32000, 50):
        try:
            DEV.write(43, i)
            time.sleep(0.100)
            current = int(DEV.read(22)[0])
            
            # Write to console
            print(f"{i},{current}")
            
            # Write to CSV file
            csv_writer.writerow([i, current])
            
            # Flush to ensure data is written immediately
            csvfile.flush()
            
        except Exception as e:
            print(e)
            print("Error reading data from device")
            exit(1)

print(f"Data successfully saved to {csv_filename}")