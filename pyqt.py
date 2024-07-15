#!/usr/bin/env python3
import sys

from PyQt6 import QtCore
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QColor, QFont, QPalette
from PyQt6.QtWidgets import (
    QApplication,
    QMainWindow,
    QLabel,
    QGridLayout,
    QWidget,
    QLCDNumber,
    QProgressBar,
)
import queue
import select
import time
import threading

FONT = QFont(None, 27)

output_queue = queue.Queue()

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



class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowFlags(QtCore.Qt.WindowType.FramelessWindowHint)
        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(0, 0, 0))
        p.setColor(QPalette.ColorRole.WindowText, QColor(255, 255, 255))
        self.setPalette(p)

        self.setWindowTitle("Solar Car Dashboard")

        self.layout = QGridLayout(self)
        self.layout.setHorizontalSpacing(10)
        self.layout.setVerticalSpacing(10)

        self.mphlcd = QLCDNumber()
        self.layout.addWidget(self.mphlcd, 0, 0, 2, 2)

        self.mphlabel = QLabel()
        self.mphlabel.setFont(FONT)
        self.mphlabel.setText("mph")
        self.layout.addWidget(self.mphlabel, 0, 2, 2, 2, Qt.AlignmentFlag.AlignLeft)

        self.kmhlcd = QLCDNumber()
        self.layout.addWidget(self.kmhlcd, 2, 0, 1, 2, Qt.AlignmentFlag.AlignRight)

        self.kmhlabel = QLabel()
        self.kmhlabel.setFont(QFont(None, FONT.pointSize() // 2))
        self.kmhlabel.setText("km/h")
        self.layout.addWidget(self.kmhlabel, 2, 2, 1, 2, Qt.AlignmentFlag.AlignLeft)

        self.batlabel = QLabel()
        self.batlabel.setFont(FONT)
        self.batlabel.setText("Adapative Total Capacity (Battery):")
        self.batlabel.setWordWrap(True)
        self.layout.addWidget(self.batlabel, 3, 0, 2, 2, Qt.AlignmentFlag.AlignRight)

        self.batbar = QProgressBar()
        self.batbar.setValue(24)
        self.layout.addWidget(self.batbar, 3, 2, 2, 2)

        self.mottemplabel = QLabel()
        self.mottemplabel.setFont(FONT)
        self.mottemplabel.setText("Motor Temp.:")
        self.layout.addWidget(
            self.mottemplabel, 5, 0, 2, 1, Qt.AlignmentFlag.AlignRight
        )

        self.mottemp = QLabel()
        self.mottemp.setFont(FONT)
        self.mottemp.setText("0.00°")
        self.layout.addWidget(self.mottemp, 5, 1, 2, 1, Qt.AlignmentFlag.AlignLeft)

        self.battemplabel = QLabel()
        self.battemplabel.setFont(FONT)
        self.battemplabel.setText("Battery Temp.:")
        self.layout.addWidget(
            self.battemplabel, 5, 2, 2, 1, Qt.AlignmentFlag.AlignRight
        )

        self.battemp = QLabel()
        self.battemp.setFont(FONT)
        self.battemp.setText("0.00°")
        self.layout.addWidget(self.battemp, 5, 3, 2, 1, Qt.AlignmentFlag.AlignLeft)

        self.efflabel = QLabel()
        self.efflabel.setFont(FONT)
        self.efflabel.setText("Efficiency:")
        self.layout.addWidget(self.efflabel, 7, 0, 2, 2, Qt.AlignmentFlag.AlignRight)

        self.eff = QLabel()
        self.eff.setFont(FONT)
        self.eff.setText("n km/h")
        self.layout.addWidget(self.eff, 7, 2, 2, 2, Qt.AlignmentFlag.AlignLeft)

        container = QWidget()
        container.setLayout(self.layout)

        # Set the central widget of the Window.
        self.setCentralWidget(container)

    def set_speed(self, mph: float | int) -> None:
        self.mphlcd.display(str(int(mph)))
        self.kmhlcd.display(str(int(mph * 1.609344)))

    def set_bat(self, val: int) -> None:
        self.batbar.setValue(val)

    def set_motor_temp(self, celsius: float):
        self.mottemp.setText(f"{celsius:.2f}°")

    def set_bat_temp(self, celsius: float):
        self.battemp.setText(f"{celsius:.2f}°")

    def set_efficiency(self, mpkwh: float):
        self.eff.setText(f"{int(mpkwh)} m/kWh")

    def loop(self) -> None:
        while not output_queue.empty():
            metric = output_queue.get()
            print(f"Received metric: {metric}")  # Debugging: Print received metric

            # Simple metric handling
            try:
                key, value = metric.split(":")
                if key.strip() == "RPM":
                    rpm = int(value.strip())
                    speed = calculate_speed(rpm) * 0.62
                    self.set_speed(rpm)
                #elif key.strip() == "Pack SOC":
                #    battery_charge = int(value.strip()) / 2
                #elif key.strip() == "Voltage In":
                #    motor_voltage = int(value.strip())
                #elif key.strip() == "current":
                #    motor_current = int(value.strip())
                elif key.strip() == "Average Temperature": #battery
                    self.set_bat_temp(float(value.strip()))
                elif key.strip() == "Temp Motor":
                    self.set_motor_temp(int(value.strip()))
                elif key.strip() == "Adaptive Total Capacity":
                    self.set_bat(int(float(value.strip())/10))

            except ValueError as e:
                print(
                    f"ValueError: {e}", file=sys.stderr
                )  # Debugging: print error message

            #try:
            #    if motor_voltage and motor_current and speed:
            #        consumption = (
            #            kwh_per_100_km(motor_voltage, motor_current, speed) / 0.6
            #        )
            #except ValueError as e:
            #    print(f"ValueError: {e}", file=sys.stderr)

            #try:
            #    if hi_temp and lo_temp:
            #        battery_temp = calculate_avg_tmp(hi_temp, lo_temp)
            #except ValueError as e:
            #    print(f"ValueError: {e}", file=sys.stderr)



app = QApplication(sys.argv)

window = MainWindow()
window.show()
window.showFullScreen()

timer = QtCore.QTimer(app)
timer.timeout.connect(window.loop)
timer.start(1000)


def main() -> None:
    #output_queue = queue.Queue()

    reader_thread = threading.Thread(
        target=read_metrics_from_stdin, args=(output_queue,)
    )

    reader_thread.daemon = True
    reader_thread.start()

    app.exec()


if __name__ == "__main__":
    main()
