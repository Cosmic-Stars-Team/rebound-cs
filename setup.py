from setuptools import setup

setup(
    name="rebound-cs",
    version="0.1.0",
    description="Cosmic Stars physics extension for REBOUND",
    packages=["rebound_cs"],
    package_data={"rebound_cs": ["_cs.dll", "_cs.so", "_cs.dylib", "_cs.pyd"]},
    python_requires=">=3.8",
)
