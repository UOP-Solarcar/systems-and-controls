import sys
import threading
import queue
from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import QApplication
from ..ui import MainWindow
from ..metrics import read_metrics_from_stdin

# Single output queue definition
output_queue = queue.Queue()

def main():
    app = QApplication(sys.argv)

    # Use the global output_queue instead of creating a new one
    window = MainWindow(output_queue)
    window.show()
    # window.showFullScreen()  # Comment this out to prevent fullscreen

    timer = QTimer(app)
    timer.timeout.connect(window.loop)
    timer.start(1000)

    reader_thread = threading.Thread(
        target=read_metrics_from_stdin, 
        args=(output_queue,),
        daemon=True
    )
    reader_thread.start()

    return app.exec()
