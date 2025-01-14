from setuptools import setup, find_packages

setup(
    name="lcd",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "PyQt6",
        "pyarrow",
        "pydantic",
    ],
    entry_points={
        "console_scripts": [
            "lcd=lcd.__main__:main",
        ],
    },
)
