#include "CAN_driver.h"
#include "CAN_test.h"
#include "MCP2515_driver.h"
#include "ir_driver.h"
#include "servo_driver.h"
#include <Wire/Wire.h>
#include "motor_driver.h"
#include "controller_driver.h"
#include <stdio.h>


Controller controller;

int uart_hack(char c, FILE* f)
{
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    UDR0 = c;
    return 1;
}

//static FILE* f = FDEV_SETUP_STREAM(uart_hack, NULL, _FDEV_SETUP_READ);

void setup()
{
    CAN_init(MCP_MODE_NORMAL);

    ir_init();
    servo_init();
    motor_init();
    controller_calibrate(&controller);

    Serial.begin(9600);
    fdevopen(uart_hack, NULL);
}
#define SCORE_TIMEOUT 1000

void loop()
{
    CanMessage_t resp;
	uint32_t last, current = 0;
	uint32_t score = 0;
	uint8_t playing = 0;
    uint32_t last_ball = 0;

    while (1)
    {
        current = millis();

        resp = CAN_receive();
        if (resp.id == INPUT_ID)
        {
            servo_joystick_control((int8_t) resp.data[1]);
            controller_set_reference(&controller, (int8_t) resp.data[1]);
            CAN_print_message(&resp);
        }
		else if(resp.id == SCORE)
		{
			score = 0;
			playing = 1;
		}

        if (ir_check() == 1)
        {
            if ((millis() - last_ball) > SCORE_TIMEOUT)
            {
                CanMessage_t message;
                message.id      = SCORE;
                message.length  = 2;
                message.data[0] = score >> 8;
                message.data[1] = score;
                CAN_send(&message);
                printf("Score updated: %d\n", score);
                playing = 0;
            }
            last_ball = millis();
        }
		
        score += (current - last);

		if (playing)
			controller_pi(&controller, (int) current - last);
		
		last = current;
    }
}
