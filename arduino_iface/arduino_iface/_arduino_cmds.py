
ARDUINO_CMDS = {
    "clear_state": {
        "req": "0"
    },
    "ir_set": {
        "value": {
            "intensityup": 0,
            "intensitydown": 1,
            "off": 2,
            "on": 3,
            "red": 4,
            "green": 5,
            "blue": 6,
            "white": 7,
            "rust": 8,
            "palegreen": 9,
            "azure": 10,
            "flash": 11,
            "redorange": 12,
            "turquoise": 13,
            "eggplant": 14,
            "strobe": 15,
            "orange": 16,
            "emerald": 17,
            "purple": 18,
            "fade": 19,
            "yellow": 20,
            "jade": 21,
            "pink": 22,
            "smooth": 23,
        },
        "req": "1{value}"
    },
    "pir_get": {
        "req": "2"
    },
    "luminosity_get":  {
        "req": "3"
    },
    "key_get": {
        "value": {
            "btn1": 1,
            "btn2": 2,
            "btn3": 3,
            "btn4": 4
        },
        "req": "4{value}"
    }
}
