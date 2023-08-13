# --- Imports --- #
import sys
import zmq
import json
import time
import serial
import signal
import logging

from threading import Event

from ._arduino_cmds import ARDUINO_CMDS

# --- Helper functions --- #
def JSON_to_str(json_obj: object) -> str:
    return json.dumps(json_obj, indent=4, sort_keys=True)

def str_to_JSON(json_str: str) -> object:
    return json.loads(json_str)

def logger_get(name: str) -> object:
    log = logging.getLogger(name)
    log.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s: [%(levelname)s] %(name)s: %(message)s')
    handler.setFormatter(formatter)
    log.addHandler(handler)

    return log

class SerialIO():
    """ Serial UART I/O"""
    UART_EOL = '\n'

    def __init__(self, port: str, baudrate: int):
        self.log = logger_get("serial_io")
        self.ser = serial.Serial(port=port, baudrate=baudrate, timeout=10)
        time.sleep(5)

    def __del__(self):
        self.ser.close()

    def _serial_readline(self):
        line = ""

        while True:
            try:
                new_char = self.ser.read(1).decode()
                if new_char:
                    if new_char == self.UART_EOL:
                        break
                    else:
                        line += new_char
                else:
                    break
            except ValueError as err:
                self.log.exception(f"--- Err: {err}\n")

        return line

    def write(self, line: str):
        line += self.UART_EOL

        try:
            self.ser.write(line.encode())
        except IOError as err:
            self.log.exception(f"--- Err: {err}")

    def read(self) -> str:
        try:
            line = self._serial_readline()
        except IOError as err:
            self.log.exception(f"--- Err: {err}")

        return line.rstrip()

class ArduinoServer():
    RECV_TIMEOUT = 2500 # poller timeout
    """docstring for ArduinoServer"""
    def __init__(self, endpoint: str, serial_port: str, serial_baudrate: int = 9600):
        super().__init__()
        context = zmq.Context()
        self.poller = zmq.Poller()
        self.socket = context.socket(zmq.REP)
        self.socket.bind(endpoint)
        self.serial = SerialIO(serial_port, serial_baudrate)
        self.poller.register(self.socket, zmq.POLLIN)
        self.log = logger_get("server")

        self.stop_server = Event()
        for sig in ('TERM', 'HUP', 'INT'):
            signal.signal(getattr(signal, 'SIG' + sig), self.__sig_handler)

    # --- SIG stop handler --- #
    def __sig_handler(self, sig, frame) -> None:
        """
            Interrupt signal handler to stop the 'run' (execution) of
            the service in use
        """
        del sig, frame
        # -- Stop the service
        self.stop_server.set()

    def format_response(self, status: str, value: str = "") -> dict:
        response = {}
        response['status'] = status
        if value != "":
            response['value'] = value

        return response


    def run(self) -> None:
        items = []
        self.log.info("Server awaiting for new requests ...")
        while not self.stop_server.is_set():
            try:
                # Ge the items list
                items = self.poller.poll(self.RECV_TIMEOUT)
            except zmq.ContextTerminated as warn:
                self.log.warning(f"Context terminated: {warn}")
            except zmq.ZMQError as warn:
                self.log.warning(f"{warn}")
            except zmq.Again as warn:
                self.log.warning( f"EAGAIN: {warn}")
            except Exception as err:
                self.log.error(f"{err}")

            if items:
                response = {}
                request_raw = self.socket.recv()
                request = str_to_JSON(request_raw.decode())
                self.log.debug(f"→  Request: {request}")
                cmd_id = request['id']
                if cmd_id not in ARDUINO_CMDS.keys():
                    response = self.format_response("error", f"Invalid cmd id: {cmd_id}")

                # Send the command to Arduino
                arduino_req = ARDUINO_CMDS[cmd_id]['req']

                try:
                    if 'value' in request.keys():
                        arduino_req = arduino_req.format(value=ARDUINO_CMDS[cmd_id]['value'][request['value']])

                    self.log.debug(f"→  Arduino Serial Request: {arduino_req}")
                    self.serial.write(arduino_req)
                    arduino_res = self.serial.read()
                    self.log.debug(f"←  Arduino Serial Response: {arduino_res}")
                except ValueError as err:
                    response = self.format_response("error", f"Failed to send cmd: {err}!")

                if arduino_res == "-1":
                    response = self.format_response("error", f"Command failed!")
                else:
                    response = self.format_response("ok", arduino_res)

                self.log.debug(f"←  Response: {response}")
                response_raw = JSON_to_str(response).encode()
                self.socket.send(response_raw)

class ArduinoClient():
    """docstring for ArduinoClient"""
    def __init__(self, endpoint: str):
        super().__init__()
        context = zmq.Context()
        self.socket = context.socket(zmq.REQ)
        self.socket.RCVTIMEO = 10000 # in milliseconds
        self.socket.connect(endpoint)
        self.log = logger_get("client")

    def send_cmd(self, cmd: dict) -> dict:
        request = ""
        response = ""

        self.log.info(f"\n→  Request:\n{json.dumps(cmd, sort_keys=True, indent=4)}")
        try:
            request_raw = JSON_to_str(cmd).encode()
            self.socket.send(request_raw)
            response_raw = self.socket.recv()
            response = str_to_JSON(response_raw.decode())
        except Exception as err:
            self.log.error(f"--- Err: {err}")
        self.log.info(f"\n←  Response:\n{json.dumps(response, sort_keys=True, indent=4)}")

        return response
