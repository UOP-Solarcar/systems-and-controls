from PyQt6.QtWidgets import (
    QMainWindow,
    QLabel,
    QGridLayout,
    QWidget,
    QLCDNumber,
    QProgressBar,
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont, QPalette, QColor
import sys
from ..metrics import calculate_speed, kwh_per_100_km
from ..metrics.recorder import MetricsRecorder
from ..models import VehicleMetrics

FONT = QFont(None, 27)


class MainWindow(QMainWindow):
    def __init__(self, output_queue=None):
        super().__init__()
        self.output_queue = output_queue
        self.metrics_recorder = MetricsRecorder()  # Initialize the recorder
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        self.setAutoFillBackground(True)

        # Set a fixed size for the window (adjust dimensions as needed)
        self.setFixedSize(800, 480)  # Common LCD display resolution

        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(0, 0, 0))
        p.setColor(QPalette.ColorRole.WindowText, QColor(255, 255, 255))
        self.setPalette(p)

        # Center the window on the screen
        screen = self.screen().geometry()
        self.setGeometry(
            (screen.width() - self.width()) // 2,
            (screen.height() - self.height()) // 2,
            self.width(),
            self.height(),
        )

        self.setWindowTitle("Solar Car Dashboard")

        # Create a central widget and set the layout to it
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        # Create layout and set it to the central widget
        self.layout = QGridLayout(central_widget)
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
        self.eff.setText("0 kWh / 100 mi")
        self.layout.addWidget(self.eff, 7, 2, 2, 2, Qt.AlignmentFlag.AlignLeft)

        self.vehicle_metrics = VehicleMetrics()

    def set_speed(self, mph: float | int) -> None:
        self.mphlcd.display(str(int(mph)))
        self.kmhlcd.display(str(int(mph * 1.609344)))

    def set_bat(self, val: int) -> None:
        self.batbar.setValue(val)

    def set_motor_temp(self, celsius: float) -> None:
        self.mottemp.setText(f"{celsius:.2f}°")

    def set_bat_temp(self, celsius: float) -> None:
        self.battemp.setText(f"{celsius:.2f}°")

    def set_efficiency(self, mpkwh: float) -> None:
        self.eff.setText(f"{int(mpkwh)} kW / 100 mi")

    def loop(self) -> None:
        if self.output_queue is None:
            return

        wh_used = 0
        wh_charged = 0
        speed = 0
        while not self.output_queue.empty():
            try:
                metric = self.output_queue.get()
                print(f"Received metric: {metric}")

                # Skip malformed metrics
                if ":" not in metric:
                    continue

                # Split only on the first occurrence of ":"
                key, value = metric.split(":", 1)
                key = key.strip()
                value = value.strip()

                # Record the metric
                self.metrics_recorder.record_metric(key, value)

                # Simple metric handling
                try:
                    # First convert value to float to handle decimal values
                    float_value = float(value)
                    
                    if key == "RPM":
                        speed = calculate_speed(float_value) * 0.62
                        self.set_speed(speed)
                    elif key == "Wh Used":
                        wh_used = int(float_value)  # Convert to int after float parsing
                    elif key == "Wh Charged":
                        wh_charged = int(float_value)  # Convert to int after float parsing
                    elif key == "Average Temperature":  # battery
                        self.set_bat_temp(float_value)
                    elif key == "Temp Motor":
                        self.set_motor_temp(float_value)
                    elif key == "Adaptive Total Capacity":
                        self.set_bat(int(float_value / 10))

                    if wh_used and wh_charged and speed:
                        consumption = kwh_per_100_km(wh_used, wh_charged, speed)
                        self.set_efficiency(int(consumption))

                except ValueError as e:
                    print(
                        f"ValueError processing metric {key}={value}: {e}",
                        file=sys.stderr,
                    )

            except ValueError as e:
                print(f"ValueError parsing metric line: {e}", file=sys.stderr)

    def update_metrics(self, metrics: VehicleMetrics) -> None:
        """Update display with new metrics"""
        self.vehicle_metrics = metrics

        if self.vehicle_metrics.motor.rpm is not None:
            speed = calculate_speed(self.vehicle_metrics.motor.rpm)
            self.set_speed(speed)

        if self.vehicle_metrics.battery.adaptive_total_capacity is not None:
            self.set_bat(self.vehicle_metrics.battery.adaptive_total_capacity)

        if self.vehicle_metrics.battery.average_temperature is not None:
            self.set_bat_temp(self.vehicle_metrics.battery.average_temperature)

        # ... rest of update logic ...
