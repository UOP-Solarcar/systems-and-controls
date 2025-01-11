import sys
import queue
import select
import time
from .parser import parse_line


def read_metrics_from_stdin(output_queue: queue.Queue) -> None:
    while True:
        try:
            rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
            if rlist:
                line = sys.stdin.readline().strip()
                if line:
                    # Parse the line and get metrics
                    metrics_list = parse_line(line)
                    # Put each metric in the queue separately
                    for metric_dict in metrics_list:
                        for key, value in metric_dict.items():
                            output_queue.put(f"{key}: {value}")
        except Exception as e:
            print(f"Error in reader thread: {e}")
        time.sleep(0.1)


def calculate_speed(rpm: int, wheel_diameter: float = 0.6) -> float:
    return (rpm * wheel_diameter * 3.141593 * 60) / 1000


def kwh_per_100_km(wh_used: int, wh_charged: int, speed: float) -> float:
    power = (wh_used - wh_charged) / 1000
    time_hours_per_100_km = 100 / speed
    energy_consumption = power * time_hours_per_100_km
    return energy_consumption
