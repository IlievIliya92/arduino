/**
 * MIT License
 *
 * Copyright (c) 2023 Iliya Iliev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

/*************************** Include modules ***************************/
#include <IRremote.h>

/******************************* Defines *******************************/
#define I_CMD_LEN_MAX 200
#define I_CMDS_N  5
#define IR_CMDS_N 24
#define PAD_KEYS_N 4

// #define ENABLE_IR_RECV
/*** PIN MUX ***/
/* Digital */
#ifdef ENABLE_IR_RECV
#define RECV_PIN 2
#endif
#define SEND_PIN 3
#define PIR_SENSOR_PIN 4
#define KEY_PAD_4 9 
#define KEY_PAD_3 8 
#define KEY_PAD_2 11
#define KEY_PAD_1 10

#define LED_PIN 13

/* Analog */
#define PHOTO_RESISTOR_SENSOR_PIN A0

/****************************** Typedefs *******************************/
typedef struct _cmd_t {
  uint8_t id;
  uint8_t pl[];
} cmd_t;

typedef void (*cmd_handler_t) (uint8_t *, uint8_t);

void clear_state_cmd_handler(uint8_t *cmd, uint8_t  cmd_len);
void ir_cmd_handler(uint8_t *cmd, uint8_t  cmd_len);
void pir_cmd_handler(uint8_t *cmd, uint8_t  cmd_len);
void photo_resistor_cmd_handler(uint8_t *cmd, uint8_t  cmd_len);
void key_pad_cmd_handler(uint8_t *cmd, uint8_t  cmd_len);
/**************************** Global Data *****************************/
/* Command Input */
static uint8_t i_cmd[I_CMD_LEN_MAX];
static uint8_t i_cmd_len = 0;
static bool i_cmd_complete = false;

/* PIR */
static volatile int pir_state = LOW; // we start, assuming no motion detected           

/* IR */
#ifdef ENABLE_IR_RECV
IRrecv irrecv(RECV_PIN);
#endif
IRsend irsend(SEND_PIN);

/* KEY PAD */
static volatile int key_state[PAD_KEYS_N] = {1, 1, 1, 1};
static volatile int key_value[PAD_KEYS_N] = {1, 1, 1, 1};
static bool key_state_read[PAD_KEYS_N] = {true, true, true, true};

cmd_handler_t i_cmd_handlers[I_CMDS_N] = {
    clear_state_cmd_handler,
    ir_cmd_handler,
    pir_cmd_handler,
    photo_resistor_cmd_handler,
    key_pad_cmd_handler    
};

const unsigned long ir_commands[IR_CMDS_N] = {
  0xFF00EF00, 0xFE01EF00, 0xFD02EF00, 0xFC03EF00,
  0xFB04EF00, 0xFA05EF00, 0xF906EF00, 0xF807EF00,
  0xF708EF00, 0xF609EF00, 0xF50AEF00, 0xF40BEF00,
  0xF30CEF00, 0xF20DEF00, 0xF10EEF00, 0xF00FEF00,
  0xEF10EF00, 0xEE11EF00, 0xED12EF00, 0xEC13EF00,
  0xEB14EF00, 0xEA15EF00, 0xE916EF00, 0xE817EF00
};

/************************** Initalization *****************************/
void setup(){
  /* LED */
  pinMode(LED_PIN, OUTPUT);
  
  /* IR */
#ifdef ENABLE_IR_RECV
  irrecv.enableIRIn();
#endif
  irsend.enableIROut(38);

  /* PIR */
  pinMode(PIR_SENSOR_PIN, INPUT);

  /* Photo Resistor */
  pinMode(PHOTO_RESISTOR_SENSOR_PIN, INPUT);

  /* Key PAD */
  pinMode(KEY_PAD_4, INPUT_PULLUP);
  pinMode(KEY_PAD_3, INPUT_PULLUP);
  pinMode(KEY_PAD_2, INPUT_PULLUP);
  pinMode(KEY_PAD_1, INPUT_PULLUP);
  
  Serial.begin(9600, SERIAL_8N1);
}

/************************** Helper Functions **************************/
void send_response(int reponse_value) {
  Serial.println(reponse_value, DEC);
}

/************************** Command Handlers **************************/
void clear_state_cmd_handler(uint8_t *cmd, uint8_t  cmd_len) {

  int i = 0;
  for (i = 0; i < PAD_KEYS_N; i++) {
    key_state[i] = 1;
    key_value[i] = 1;
    key_state_read[i] = true;
  }
  pir_state = LOW;
  send_response(0);
}

void ir_cmd_handler(uint8_t *cmd, uint8_t  cmd_len) {
  uint8_t ir_cmd_id = 0;

  ir_cmd_id = atoi(cmd);
  if (ir_cmd_id >= 0 && ir_cmd_id < IR_CMDS_N) {  
    unsigned long ir_out_value = ir_commands[ir_cmd_id];
    irsend.sendNECRaw(ir_out_value, 32);
#ifdef ENABLE_IR_RECV
    if (irrecv.decode()){
      // Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
      irrecv.resume();
    }
#endif
    send_response(0);
  } else {
    send_response(-1);
    //Serial.print("Invalid IR command id: ");
    //Serial.println(ir_cmd_id, DEC);
  }
}

void pir_cmd_handler(uint8_t *cmd, uint8_t  cmd_len) {
  send_response(pir_state);
  digitalWrite(LED_PIN, LOW); // turn LED OFF
  pir_state = LOW;
}

void photo_resistor_cmd_handler(uint8_t *cmd, uint8_t  cmd_len) {
  int val = -1;
  val = analogRead(PHOTO_RESISTOR_SENSOR_PIN);
  send_response(val);
}

void pir_poll(void) {
  int val = digitalRead(PIR_SENSOR_PIN);
  if (val == HIGH) {        
    digitalWrite(LED_PIN, HIGH); 
    if (pir_state == LOW) {
      //Serial.println("Motion detected!");
      pir_state = HIGH;
    }
  }  
}

void key_pad_cmd_handler(uint8_t *cmd, uint8_t  cmd_len) {
  uint8_t key_id = 0;

  key_id = atoi(cmd);
  if (key_id >= 1 && key_id < 5) {  
    key_id = key_id - 1;
    send_response(key_state[key_id]);
    key_state[key_id] = 1;
    key_state_read[key_id] = true;
  } else {
    send_response(-1);
    //Serial.print("Invalid KEY ID id: ");
    //Serial.println(key_id, DEC);
  }
}

void key_pad_poll(void) {
  int i = 0;

  key_value[0] = digitalRead(KEY_PAD_1);
  key_value[1] = digitalRead(KEY_PAD_2);
  key_value[2] = digitalRead(KEY_PAD_3);
  key_value[3] = digitalRead(KEY_PAD_4);

  for (i = 0; i < 4; i ++) {
    if (key_state[i] != key_value[i] && key_state_read[i]) {
      key_state[i] = key_value[i];
      key_state_read[i] = false;     
    } 
  }
}

/****************************** Main Loop *****************************/
void loop(){
  pir_poll();
  key_pad_poll();

  if (i_cmd_complete) {
      cmd_t *cmd = NULL;
      cmd = (cmd_t *)i_cmd;

      uint8_t cmd_id = cmd->id - '0';
      if (cmd_id >= 0 && cmd_id < I_CMDS_N) {
        i_cmd_handlers[cmd_id](cmd->pl, (i_cmd_len - 1));
        i_cmd_len = 0;
      } else {
        Serial.println("-1");
        Serial.print("Invalid command id: ");
        Serial.println(cmd_id, DEC);
      }

      i_cmd_complete = false;
  }

  delay(100);
}

/**************************** Serial Event ****************************/
/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    //Serial.println(inChar, HEX);

    if (i_cmd_len == I_CMD_LEN_MAX - 1 ) {
      i_cmd_complete = true;
    } else {
      i_cmd[i_cmd_len] = inChar;
      i_cmd_len++;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == '\n') {
        i_cmd_complete = true;
      }
    }

  }
}
