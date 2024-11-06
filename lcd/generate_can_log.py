from datetime import datetime, timedelta

# Define the output file path
output_file = "can_sim_test.txt"

# Define a starting timestamp
start_time = datetime(2023, 1, 1, 0, 0, 0)

# Open the file for writing
with open(output_file, 'w') as f:
    # Define the number of rows to generate
    num_rows = 1 # Adjust as needed

    # Loop to alternate between BMS and Motor Controller entries
    for i in range(num_rows):
        # Calculate timestamp for each row
        timestamp = start_time + timedelta(seconds=i)

        # Write either BMS or Motor Controller entry based on row index
        # BMS data for even rows
        f.write(
            f"Cell ID: {i%20} Instant Voltage: {(i%5)*2} Internal Resistance: {1509 - i%10} Open Voltage: {0} Checksum: {63 + i}\n"
            f"Motor Controller ID: 0x21 Status #3 Wh Used: {2000 + i * 50} Wh Charged: {5000 + i * 65}\n"
            f"Pack Current: {3 + i%20} Pack Inst. Voltage: {0} Pack SOC: {20 + i % 30} Relay State: {32840 + i} Checksum: {130 + i}\n"
            f"Motor Controller ID: 0x21 Status #1 RPM: {i%40 * 10} current: {20 + (i%5)} duty cycle: {3 + i % 10}\n"
            f"Motor Controller ID: 0x21 Status #5 Tachometer: {600 + i * .25} Voltage In: {50 + i%15 * 5}\n"
            f"High Temperature: {25 + i % 5} High Thermistor ID: {i%30} Low Temperature: {23} Low Thermistor ID: {4} Checksum: {60 + i}\n"
            f"Motor Controller ID: 0x21 Status #6 ADC1: {600 + i * 5} ADC2: {620 + i * 3} ADC3: {610 + i * 4} PPM: {0}\n"
            f"Motor Controller ID: 0x21 Status #2 Ah Used: {276 + i * 2} Ah Charged: {52 + i} Checksum: {100 + i}\n"
        )

print(f"Simulated CAN log saved to {output_file}")