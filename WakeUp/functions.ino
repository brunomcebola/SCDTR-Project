#include "constants.h"
#include "mixin.h"

void execute_steps(int n_steps, bool loop) {
    const int pwm_steps = ANALOG_MAX/n_steps;

    int pwm, reading;

    Serial.println("\n--- STEPS STARTED ---\n");

    Serial.println("DAC | ADC");

    do {
      for (pwm = 0; pwm <= ANALOG_MAX; pwm += pwm_steps) {
          analogWrite(LED_PIN, pwm);  // sets LED to ADC_write intensity
          delay(1000);

          reading = analogRead(A0);  // read LDR received intensity

          Serial.printf("%d %n\n", pwm, reading);
      }
    } while(loop);    

    Serial.println("\n--- STEPS ENDED ---\n");
}

void execute_comb_step_micro_readings(int n_steps) {
    const int on_samples = 5000;
    const int off_samples = 1000;
    const int pwm_steps = ANALOG_MAX/n_steps;

    int pwm;
    float reading;
    int time_aux = 0;

    for (pwm = 0; pwm <= ANALOG_MAX; pwm += pwm_steps) {
        analogWrite(LED_PIN, 0);  // set led PWM

        for (int i = 0; i < off_samples; i++) {
            time_aux = micros();
            reading = n_to_volt(analogRead(A0));

            Serial.printf("%d %.6f\n", time_aux, reading);
            delay(1);
        }

        analogWrite(LED_PIN, pwm);  // set led PWM

        for (int i = 0; i < on_samples; i++) {
            time_aux = micros();
            reading = n_to_volt(analogRead(A0));

            Serial.printf("%d %.6f\n", time_aux, reading);
        }
    }
}

float calibrate_gain() {
    int x0 = ANALOG_MAX / 8;
    int x1 = ANALOG_MAX;
    float y0, y1, G;

    Serial.println("\n--- GAIN CALIBRATION STARTED ---\n");

    // turns LED on to max intensity
    analogWrite(LED_PIN, ANALOG_MAX);  
    delay(1000);

    // turns LED off
    analogWrite(LED_PIN, 0);  
    delay(1000);

    // sets LED to x0 intensity
    analogWrite(LED_PIN, x0);  
    delay(1000);
    y0 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

    // sets LED to x1 intensity
    analogWrite(LED_PIN, x1);  
    delay(1000);
    y1 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

    // turns LED off
    analogWrite(LED_PIN, 0);  
    delay(1000);
    
    // gain computation
    G = (y1 - y0) / (x1 - x0);
   
    Serial.println("\n--- GAIN CALIBRATION ENDED ---\n");

    return G;
}

/****************************
*          SECOND PART      *
*****************************/

void calibrateOwnGain(int crossedId){
  int x0 = ANALOG_MAX / 8;
  int x1 = ANALOG_MAX;
  float y0, y1, G;
  Serial.println("ENTREI NOS OWN GANHOS");
  // turns LED on to max intensity
  analogWrite(LED_PIN, ANALOG_MAX);  
  delay(500);

  // turns LED off
  analogWrite(LED_PIN, 0);  
  delay(500);

  // sets LED to x0 intensity
  analogWrite(LED_PIN, x0);  
  delay(500);
  y0 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

  // sets LED to x1 intensity
  analogWrite(LED_PIN, x1);  
  delay(500);
  y1 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

  // turns LED off
  analogWrite(LED_PIN, 0);  
  delay(500);
    
  // gain computation
  k[crossedId - 1] = (y1 - y0) / (x1 - x0);
}

void calibrateCrossGain(int crossedId){
  int x0 = ANALOG_MAX / 8;
  int x1 = ANALOG_MAX;
  float y0, y1, G;
  Serial.print(crossedId); Serial.print(" ");
  Serial.println("ENTREI NOS GANHOS");
  
  // turns LED on to max intensity
  delay(500);

  // turns LED off
  delay(500);

  // sets LED to x0 intensity
  delay(500);
  y0 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

  // sets LED to x1 intensity
  delay(500);
  y1 = ldr_volt_to_lux(n_to_volt(analogRead(A0)), M, B);

  // turns LED off
  delay(500);
    
  // gain computation
  k[crossedId - 1] = (y1 - y0) / (x1 - x0);
}

// When Slave sends data do this
void recv(int len) {
  
  int i, n_available_bytes;
  byte rx_buf[frame_size]={0};
  my_i2c_msg msg;

  n_available_bytes = Wire.available();
  if(n_available_bytes != len){
    // Do Something for Debug...
    if(len > frame_size) {
    for (i = frame_size; i < len; i++) Wire1.read(); // Flush
    n_frame_errors_overrun++;
    error_frame_size = len;
    }
    
    if(len < frame_size) {
        n_frame_errors_underrun++;
        error_frame_size = len;
    }
  }
  
  //storing msg recieved in the input_fifo
  for (i = 0; i < len; i++) rx_buf[i] = Wire1.read();
  
  memcpy(&msg, rx_buf, msg_size);
  
  for(i = 0; i < input_fifo_size; i++){
    if(!input_slot_full[i]){
      input_slot_full[i] = true;
      input_fifo[i].node = msg.node;
      input_fifo[i].ts = msg.ts;
      input_fifo[i].value = msg.value;
      input_fifo[i].cmd[0] = msg.cmd[0];
      input_fifo[i].cmd[1] = msg.cmd[1];
      input_fifo[i].cmd[2] = msg.cmd[2];
      break;
    }
  }

  if( i == input_fifo_size ) n_overflows++;

  switch (msg.cmd[0]){
    case '!': // calibration call
        Serial.println(msg.cmd);
        if(msg.cmd[1] == (char) (48 + ID) ){
            calibrateOwnGain( msg.cmd[1] - 48);
        }
        else{
            calibrateCrossGain(  msg.cmd[1] - 48 );
        }
          Serial.println("DENTRO DO REV");
          Serial.print(k[0], 6); Serial.print(" ");
          Serial.print(k[1], 6); Serial.print(" ");
          Serial.println(k[2], 6);
      break;
    case '?':
      Serial.println("STARTING THE HUB");
      break;  
    default:
      Serial.println(msg.cmd[1]);
      Serial.println("ERROR ON RECEIVING CALL, COMMAND NOT FOUND");
      break;
  }  
}

// When Master requested data do this
void req(void){
  byte * px = (byte*) &k[0];
  byte new_message[12];
  Serial.println("ENTREI NO REQUEST");
  if(ID == 1){
    delay(2);
    Serial.println("ENVIEI K 1 ");
    memcpy(new_message, px , 4);
    Wire1.write(new_message, 4);
  }
  else if(ID == 2){
    delay(2);
    Serial.println("ENVIEI K 2 ");
    memcpy(new_message, k , 12);
    Wire1.write(new_message, 12);
  }
}

//NEEDS TO BE CHECKED
int masterTransmission(uint8_t transmission_addr, byte message[]){

  Wire.beginTransmission(transmission_addr);
  Wire.write(message, frame_size);
  return Wire.endTransmission();
}

//NEEDS TO BE CHECKED
int slaveTransmission(uint8_t transmission_addr, byte message[]){

  Wire1.write( message , 12); // ID - 1, because id starts in 1
  return Wire1.endTransmission();
}

void calibrationGain(){
  char print_buf[200];
  byte tx_buf[frame_size];
  uint8_t i2c_broadcast_addr = 0x00;
  char aux[3];

  // calculte the gains matrix (k)
  for(int i = 1; i <= MAX_IDS; i++){
    aux[0] = '!';
    aux[1] = 48 + i;
    my_i2c_msg tx_msg{ i2c_address, millis(), 0 };
    strcpy(tx_msg.cmd,  aux);

    memcpy(tx_buf, &tx_msg, msg_size);

    masterTransmission(0b0001111 + i, tx_buf);
    delay(2500); // wait seconds, that represent the ammount of time needed for calibration
  }
  // request the vectors for the gains matrix (k)

}
