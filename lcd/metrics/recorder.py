from lcd_metrics import MetricsRecorder as RustMetricsRecorder
import signal
import atexit


class MetricsRecorder:
    def __init__(self, output_dir: str = "data/recordings"):
        self.recorder = RustMetricsRecorder(output_dir)

        # Register save handlers
        atexit.register(self.save_to_csv)
        signal.signal(signal.SIGTERM, self._signal_handler)
        signal.signal(signal.SIGINT, self._signal_handler)

    def record_metric(self, key: str, value: str) -> None:
        """Record a metric with current timestamp"""
        self.recorder.record_metric(key, value)

    def save_to_csv(self) -> None:
        """Save recorded metrics to CSV file"""
        self.recorder.save_to_csv()

    def _signal_handler(self, signum, frame):
        """Handle termination signals"""
        self.save_to_csv()
        signal.default_int_handler(signum, frame)
