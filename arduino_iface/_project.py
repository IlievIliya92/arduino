NAME        = "arduino_iface"
VERSION     = "1.0.0"
LICENSE     = "mit"
DESCRIPTION = """Arduino Serial interface"""
URL         = "https://github.com/IlievIliya92/arduino"
PACKAGES    = ["arduino_iface"]
REQUIRES    = ["pyserial", "pyzmq"]
BINARIES    = ["./bin/arduino_server"]
