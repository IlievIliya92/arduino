# --- Imports --- #
import pytest
import arduino_iface

class Test_ArduinoIface:
    """
        Test Arduino Server using its counter part Arduino Client
        We assume that the server has been started and the tests are running
        on the same machine as the server
    """
    @pytest.fixture()
    def client (self):
        return arduino_iface.ArduinoClient("tcp://127.0.0.1:4555")

    def test_clear_state(self, client):
        res = client.send_cmd({"id": "clear_state"})
        assert res['status'] == 'ok'

    @pytest.mark.parametrize("ir_value", arduino_iface.ARDUINO_CMDS['ir_set']['value'].keys())
    def test_ir_set(self, client, ir_value):
        res = client.send_cmd({"id": "ir_set", "value": ir_value})
        assert res['status'] == 'ok'

    def test_pir_get(self, client):
        res = client.send_cmd({"id": "pir_get"})
        assert res['status'] == 'ok'
        assert int(res['value']) == 0 or int(res['value']) == 1

    def test_luminosity_get(self, client):
        res = client.send_cmd({"id": "luminosity_get"})
        assert res['status'] == 'ok'
        assert int(res['value']) > 0 and int(res['value']) < 1024

    @pytest.mark.parametrize("key", arduino_iface.ARDUINO_CMDS['key_get']['value'].keys())
    def test_key_get(self, client, key):
        res = client.send_cmd({"id": "key_get", "value": key})
        assert res['status'] == 'ok'
        assert int(res['value']) == 0 or int(res['value']) == 1
