#! /usr/bin/env python3
import pygame
import sys
import select
import threading
import queue
import time

# Initialize Pygame
pygame.init()
screen = pygame.display.set_mode((800, 480))
pygame.display.set_caption("Motor Controller Dashboard")
font = pygame.font.Font(None, 36)
clock = pygame.time.Clock()

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)


def read_metrics_from_stdin(output_queue):
    print("Reader thread started")  # Debugging: Confirm thread start
    while True:
        try:
            rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
            if rlist:
                line = sys.stdin.readline()
                if line.strip():
                    print(f"Read line: {line.strip()}")  # Debugging: Print read line
                    output_queue.put(line.strip())
        except Exception as e:
            print(f"Error in reader thread: {e}")
        time.sleep(0.1)


def calculate_speed(rpm, wheel_diameter=0.6):
    return (rpm * wheel_diameter * 3.141593 * 60) / 1000  # Adjusted formula


def calculate_avg_tmp(high_temp, low_temp):
    return (high_temp + low_temp) / 2


def kwh_per_100_km(voltage, current, speed):
    power = voltage * current
    time_hours_per_100_km = 100 / speed
    energy_consumption = power * time_hours_per_100_km
    return energy_consumption


def display_metrics(
    rpm,
    speed,
    battery_charge,
    consumption,
    motor_temp,
    battery_temp,
    adaptive_total_capacity,
):
    screen.fill(BLACK)
    # Render the speed
    speed_text = font.render(f"Speed: {speed:.2f} mph", True, WHITE)
    speed_rect = speed_text.get_rect(center=(400, 117))  # Centered the text
    screen.blit(speed_text, speed_rect)

    # Render the RPM
    rpm_text = font.render(f"RPM: {rpm}", True, WHITE)
    rpm_rect = rpm_text.get_rect(center=(400, 195))
    screen.blit(rpm_text, rpm_rect)

    # Render the State of Charge (SOC)
    capacity_text = font.render(f"Battery Charge: {battery_charge:.2f}%", True, WHITE)
    capacity_rect = capacity_text.get_rect(center=(200, 273))  # Centered the text
    screen.blit(capacity_text, capacity_rect)

    # Render the motor consumption
    consumption_text = font.render(
        f"Consumption: {consumption:.2f} kWh/100 mi", True, WHITE
    )
    consumption_rect = consumption_text.get_rect(center=(200, 352))
    screen.blit(consumption_text, consumption_rect)

    # Render the motor temperature
    motortemp_text = font.render(f"Motor Temperature: {motor_temp:.2f} °C", True, WHITE)
    motortemp_rect = motortemp_text.get_rect(center=(600, 273))
    screen.blit(motortemp_text, motortemp_rect)

    # Render the battery temperature
    batt_temp_text = font.render(
        f"Battery Temperature: {battery_temp:.2f} °C", True, WHITE
    )
    batt_temp_rect = batt_temp_text.get_rect(center=(600, 352))
    screen.blit(batt_temp_text, batt_temp_rect)

    # Render the Adaptive Total Capacity
    adaptive_total_capacity_text = font.render(
        f"Adaptive Total Capacity: {adaptive_total_capacity:.2f} Ah", True, WHITE
    )
    adaptive_total_capacity_rect = adaptive_total_capacity_text.get_rect(
        center=(400, 416)
    )
    screen.blit(adaptive_total_capacity_text, adaptive_total_capacity_rect)

    pygame.display.flip()


def main():
    output_queue = queue.Queue()

    reader_thread = threading.Thread(
        target=read_metrics_from_stdin, args=(output_queue,)
    )
    reader_thread.daemon = True
    reader_thread.start()

    print("Starting main loop...")  # Debugging: Print when the main loop starts

    rpm = 0
    speed = 0
    battery_charge = 0
    adaptive_total_capacity = 0
    motor_voltage = 0
    motor_current = 0
    consumption = 0
    battery_temp = 0
    hi_temp = 0
    lo_temp = 0
    motor_temp = 0

    try:
        while True:
            while not output_queue.empty():
                metric = output_queue.get()
                print(f"Received metric: {metric}")  # Debugging: Print received metric

                # Simple metric handling
                try:
                    key, value = metric.split(":")
                    if key.strip() == "RPM":
                        rpm = int(value.strip())
                        speed = calculate_speed(rpm) * 0.62
                    elif key.strip() == "Pack SOC":
                        battery_charge = int(value.strip()) / 2
                    elif key.strip() == "Voltage In":
                        motor_voltage = int(value.strip())
                    elif key.strip() == "current":
                        motor_current = int(value.strip())
                    elif key.strip() == "High Temperature":
                        hi_temp = int(value.strip())
                    elif key.strip() == "Low Temperature":
                        lo_temp = int(value.strip())
                    elif key.strip() == "Temp Motor":
                        motor_temp = int(value.strip())
                    elif key.strip() == "Adaptive Total Capacity":
                        adaptive_total_capacity = int(value.strip()) / 10

                except ValueError as e:
                    print(
                        f"ValueError: {e}", file=sys.stderr
                    )  # Debugging: print error message

                try:
                    if motor_voltage and motor_current and speed:
                        consumption = (
                            kwh_per_100_km(motor_voltage, motor_current, speed) / 0.6
                        )
                except ValueError as e:
                    print(f"ValueError: {e}", file=sys.stderr)

                try:
                    if hi_temp and lo_temp:
                        battery_temp = calculate_avg_tmp(hi_temp, lo_temp)
                except ValueError as e:
                    print(f"ValueError: {e}", file=sys.stderr)

                print(
                    f"Updated metrics - Speed: {speed}, Battery Charge: {battery_charge}, Consumption: {consumption}, Battery Temp: {battery_temp}, Motor Temp: {motor_temp}"
                )  # Debugging: print updated values

            display_metrics(
                rpm,
                speed,
                battery_charge,
                consumption,
                motor_temp,
                battery_temp,
                adaptive_total_capacity,
            )
            clock.tick(30)  # Update the display at 30 FPS

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    raise KeyboardInterrupt
    except KeyboardInterrupt:
        pygame.quit()
        print("Exiting...")  # Debugging: Print when exiting


if __name__ == "__main__":
    main()
