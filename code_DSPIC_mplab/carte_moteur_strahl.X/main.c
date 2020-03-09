#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <libpic30.h>
#include "ChipConfig.h"
#include "IO.h"
#include "timer.h"
#include "PWM.h"
#include "ADC.h"
#include "main.h"
#include "Robot.h"
#include "UART.h"
#include "CB_TX1.h"
#include "CB_RX1.h"

//GITHUB CHECK COMMENT 03/05

//robot params
/*
int FowardSpeed = 40;
int RotationSpeed = 10;
int CompWheelRotSpeed = 10;
int SlowDownSpeed = 20;
int TriggerRotationDistance[5] = {25, 30, 30, 30, 25};
int TriggerSlowDownDistance[5] = {35, 40, 40, 40, 35};
 */

int FowardSpeed = 40;
int RotationSpeed = 15;
int CompWheelRotSpeed = 10;
int SlowDownSpeed = 20;
int TriggerRotationDistance[5] = {25, 30, 35, 30, 25};
int TriggerSlowDownDistance[5] = {35, 40, 45, 40, 35};
int rotCount = 0;

unsigned char doDelay = 0x00;

unsigned char stateRobot;

void exitCorridor(unsigned char direction) {
    if (direction == DROITE && JACK) {
        PWMSetSpeedConsigne(-5, MOTEUR_DROIT);
        PWMSetSpeedConsigne(15, MOTEUR_GAUCHE);
    } else {
        PWMSetSpeedConsigne(15, MOTEUR_DROIT);
        PWMSetSpeedConsigne(-5, MOTEUR_GAUCHE);
    }
}

void failSafeRotate()
{
    PWMSetSpeedConsigne(40, MOTEUR_DROIT);
    PWMSetSpeedConsigne(-40, MOTEUR_DROIT);
    __delay32(20000000);
}

void OperatingSystemLoop(void) {
    if (JACK) {
        if(doDelay)
        { __delay32(2 * 40000000); doDelay = 0; }
        
        switch (stateRobot) {
            case STATE_ATTENTE:
                timestamp = 0;
                PWMSetSpeedConsigne(0, MOTEUR_DROIT);
                PWMSetSpeedConsigne(0, MOTEUR_GAUCHE);
                stateRobot = STATE_ATTENTE_EN_COURS;

            case STATE_ATTENTE_EN_COURS:
                if (timestamp > 1000)
                    stateRobot = STATE_AVANCE;
                break;

            case STATE_AVANCE:
                rotCount = 0;
                PWMSetSpeedConsigne(FowardSpeed, MOTEUR_DROIT);
                PWMSetSpeedConsigne(FowardSpeed, MOTEUR_GAUCHE);
                stateRobot = STATE_AVANCE_EN_COURS;
                LED_BLEUE = 1;
                LED_ORANGE = 0;
                LED_BLANCHE = 0;
                break;
            case STATE_AVANCE_EN_COURS:
                SetNextRobotStateInAutomaticMode();
                break;

            case STATE_TOURNE_GAUCHE:
                rotCount++;
                PWMSetSpeedConsigne(RotationSpeed, MOTEUR_DROIT);
                PWMSetSpeedConsigne(-CompWheelRotSpeed, MOTEUR_GAUCHE);
                stateRobot = STATE_TOURNE_GAUCHE_EN_COURS;
                LED_BLEUE = 0;
                LED_ORANGE = 0;
                LED_BLANCHE = 1;
                break;
            case STATE_TOURNE_GAUCHE_EN_COURS:
                SetNextRobotStateInAutomaticMode();
                break;

            case STATE_TOURNE_DROITE:
                rotCount++;
                PWMSetSpeedConsigne(-CompWheelRotSpeed, MOTEUR_DROIT);
                PWMSetSpeedConsigne(RotationSpeed, MOTEUR_GAUCHE);
                stateRobot = STATE_TOURNE_DROITE_EN_COURS;
                LED_BLEUE = 0;
                LED_ORANGE = 1;
                LED_BLANCHE = 0;
                break;
            case STATE_TOURNE_DROITE_EN_COURS:
                SetNextRobotStateInAutomaticMode();
                break;

            case STATE_TOURNE_SUR_PLACE_GAUCHE:
                rotCount = 0;
                PWMSetSpeedConsigne((timestamp % 2 != 0) ? (RotationSpeed) : (-RotationSpeed), MOTEUR_DROIT);
                PWMSetSpeedConsigne((timestamp % 2 != 0) ? (-RotationSpeed) : (RotationSpeed), MOTEUR_GAUCHE);
                stateRobot = STATE_TOURNE_SUR_PLACE_GAUCHE_EN_COURS;
                LED_BLEUE = 1;
                LED_ORANGE = 1;
                LED_BLANCHE = 1;
                break;
            case STATE_TOURNE_SUR_PLACE_GAUCHE_EN_COURS:
                SetNextRobotStateInAutomaticMode();
                break;

            case STATE_TOURNE_SUR_PLACE_DROITE:
                rotCount = 0;
                PWMSetSpeedConsigne(-RotationSpeed, MOTEUR_DROIT);
                PWMSetSpeedConsigne(RotationSpeed, MOTEUR_GAUCHE);
                stateRobot = STATE_TOURNE_SUR_PLACE_DROITE_EN_COURS;
                break;

            case STATE_TOURNE_SUR_PLACE_DROITE_EN_COURS:
                SetNextRobotStateInAutomaticMode();
                break;

            case STATE_SLOW_DOWN:
                rotCount = 0;
                PWMSetSpeedConsigne(SlowDownSpeed, MOTEUR_GAUCHE);
                PWMSetSpeedConsigne(SlowDownSpeed, MOTEUR_DROIT);
                SetNextRobotStateInAutomaticMode();
                break;

            default:
                stateRobot = STATE_ATTENTE;
                break;
        }
    } else {
        doDelay = 1;
        PWMSetSpeedConsigne(0, MOTEUR_DROIT);
        PWMSetSpeedConsigne(0, MOTEUR_GAUCHE);
        timestamp = 0;
        stateRobot = STATE_AVANCE;
    }
}

unsigned char nextStateRobot = 0;

void SetNextRobotStateInAutomaticMode() {
    unsigned char positionObstacle = PAS_D_OBSTACLE;

    if(rotCount >= 4)
    { failSafeRotate(); rotCount = 0; }
    
    if ((robotState.distanceTelemetre3 < (TriggerRotationDistance[3] - 12)) && (robotState.distanceTelemetre1 < (TriggerRotationDistance[1] - 12))) {
        if (robotState.distanceTelemetre2 < (TriggerRotationDistance[2] - 12))
            positionObstacle = OBSTACLE_EN_FACE;
        else
            exitCorridor((robotState.distanceTelemetre3 < robotState.distanceTelemetre1) ? (DROITE) : (GAUCHE));
    }

    else if (robotState.distanceTelemetre2 < TriggerRotationDistance[2]) //Obstacle en face
        positionObstacle = OBSTACLE_EN_FACE;

    else if (((robotState.distanceTelemetre3 < TriggerRotationDistance[3]) || (robotState.distanceTelemetre4 < TriggerRotationDistance[4])) ||
            ((robotState.distanceTelemetre2 < TriggerRotationDistance[2]) && (robotState.distanceTelemetre4 < TriggerRotationDistance[4]))
            ) //Obstacle � droite
        positionObstacle = OBSTACLE_A_GAUCHE;

    else if (((robotState.distanceTelemetre1 < TriggerRotationDistance[1]) || (robotState.distanceTelemetre0 < TriggerRotationDistance[0])) ||
            ((robotState.distanceTelemetre0 < TriggerRotationDistance[0]) && (robotState.distanceTelemetre2 < TriggerRotationDistance[2]))) //Obstacle � gauche
        positionObstacle = OBSTACLE_A_DROITE;



    else if (
            (robotState.distanceTelemetre0 < TriggerSlowDownDistance[0] && robotState.distanceTelemetre0 > TriggerRotationDistance[0]) ||
            (robotState.distanceTelemetre1 < TriggerSlowDownDistance[1] && robotState.distanceTelemetre1 > TriggerRotationDistance[1]) ||
            (robotState.distanceTelemetre2 < TriggerSlowDownDistance[2] && robotState.distanceTelemetre2 > TriggerRotationDistance[2]) ||
            (robotState.distanceTelemetre3 < TriggerSlowDownDistance[3] && robotState.distanceTelemetre3 > TriggerRotationDistance[3]) ||
            (robotState.distanceTelemetre4 < TriggerSlowDownDistance[4] && robotState.distanceTelemetre4 > TriggerRotationDistance[4])
            )
        positionObstacle = SLOW_DOWN;



    else if ((robotState.distanceTelemetre0 > TriggerSlowDownDistance[0]) &&
            (robotState.distanceTelemetre1 > TriggerSlowDownDistance[1]) &&
            (robotState.distanceTelemetre2 > TriggerSlowDownDistance[2]) &&
            (robotState.distanceTelemetre3 > TriggerSlowDownDistance[3]) &&
            (robotState.distanceTelemetre4 > TriggerSlowDownDistance[4])) //pas d?obstacle
        positionObstacle = PAS_D_OBSTACLE;




    //D�termination de l?�tat � venir du robot
    if (positionObstacle == PAS_D_OBSTACLE)
        nextStateRobot = STATE_AVANCE;
    else if (positionObstacle == OBSTACLE_A_DROITE)
        nextStateRobot = STATE_TOURNE_GAUCHE;
    else if (positionObstacle == OBSTACLE_A_GAUCHE)
        nextStateRobot = STATE_TOURNE_DROITE;
    else if (positionObstacle == OBSTACLE_EN_FACE)
        nextStateRobot = STATE_TOURNE_SUR_PLACE_GAUCHE;
    else if (positionObstacle == SLOW_DOWN)
        nextStateRobot = STATE_SLOW_DOWN;
    //Si l?on n?est pas dans la transition de l?�tape en cours
    if (nextStateRobot != stateRobot - 1)
        stateRobot = nextStateRobot;
}

int main(void) {
    unsigned int* distance;

    //init stuff
    InitOscillator();
    InitTimer23();
    InitTimer1();
    InitTimer4();
    InitIO();
    InitPWM();
    InitADC1();
    InitUART();
    
    while (1) {
        if (ADCIsConversionFinished()) {
            ADCClearConversionFinishedFlag();
            distance = ADCGetResult();

            robotState.distanceTelemetre0 = 34 / (((float) distance[0]) * 3.3 / 4096 * 3.2) - 5;
            robotState.distanceTelemetre1 = 34 / (((float) distance[1]) * 3.3 / 4096 * 3.2) - 5;
            robotState.distanceTelemetre2 = 34 / (((float) distance[2]) * 3.3 / 4096 * 3.2) - 5;
            robotState.distanceTelemetre3 = 34 / (((float) distance[3]) * 3.3 / 4096 * 3.2) - 5;
            robotState.distanceTelemetre4 = 34 / (((float) distance[4]) * 3.3 / 4096 * 3.2) - 5;
        }
        
        /* LOOPBACK TEST
        int i;
        for(i = 0; i < CB_RX1_GetDataSize(); i++)
        {
            unsigned char c = CB_RX1_Get();
            SendMessage(&c, 1);
        }


        unsigned char text[7] = "boijour";
        UartEncodeAndSendMessage(0x0080, 7, text);
        __delay32(40000000);
        text[1] = "A";
        UartEncodeAndSendMessage(0x0080, 7, text);
        __delay32(40000000);
        text[1] = "Z";
        UartEncodeAndSendMessage(0x0080, 7, text);
        __delay32(40000000);

                 * */
       
    }
}

