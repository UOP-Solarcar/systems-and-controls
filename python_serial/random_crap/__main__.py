#!/usr/bin/env python3
import signal
from time import sleep
from types import FrameType

import RPi.GPIO as GPIO

PIN = 17


def main() -> None:
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(PIN, GPIO.OUT)

    def signal_handler(sig: int, frame: FrameType):
        GPIO.cleanup()
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, signal_handler)

    GPIO.output(PIN, True)
    while True:
        sleep(0.5)
        GPIO.output(PIN, False)
        sleep(0.5)
        GPIO.output(PIN, True)


if __name__ == "__main__":
    main()
