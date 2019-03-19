#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "crc.h"
#include "byteSelection.h"
#include <string.h>
// Declaracao do emissor

#define BEGIN_PACKAGE_INDEX_HEADER                      0
#define BEGIN_PACKAGE_INDEX_SESSION                     4   
#define BEGIN_PACKAGE_INDEX_NUMBER_PACKAGE_LSB0         5
#define BEGIN_PACKAGE_INDEX_NUMBER_PACKAGE_LSB1         6
#define BEGIN_PACKAGE_INDEX_CRC                         7
#define BEGIN_PACKAGE_LENGTH                            8

#define MAIN_PACKAGE_INDEX_FIRST_HEADER                 0
#define MAIN_PACKAGE_INDEX_SECOND_HEADER                4
#define MAIN_PACKAGE_INDEX_ID_PACKAGE_LSB0              8
#define MAIN_PACKAGE_INDEX_ID_PACKAGE_LSB1              9
#define MAIN_PACKAGE_INDEX_DUMMY                       10
#define MAIN_PACKAGE_INDEX_CRC                         19
#define MAIN_PACKAGE_LENGTH                            20

#define END_PACKAGE_INDEX_FIRST_HEADER                  0
#define END_PACKAGE_INDEX_CRC                           4
#define END_PACKAGE_LENGTH                              5

/*  BEGIN PACKAGE - {"M","N","B","G",SESSION_ID,"CONFIG","NUMBER_PACKAGE_LSB1","NUMBER_PACKAGE_LSB0",CRC};
    MAIN PACKAGE - {"M","N","M","A","M","N","M","A",DUMMY-11BYTES,CRC};
    END PACKAGE - {"M","N","E","N",CRC};
*/

#define SESSION_ID                                      0
#define TELEMETRY_FREQUENCY                         900.0

#define TOTAL_PACKAGES_NUMBER                        10

RH_RF95 emitterObject(4, 3); 


uint16_t currentNumberPackagesSent = 0;
bool     sessionSaved = false;
bool     EndPackageSent = false;

uint8_t  arrayComeBackReceived[4] = {'M','N','R','V'};
uint8_t  arraytoSignEndPackageReceived[3] = {'E', 'N', 'D'};
uint16_t id = 0;
uint8_t  bufReceived[RH_RF95_MAX_MESSAGE_LEN];
uint8_t  lenBuf = sizeof(bufReceived);

uint8_t arrayBeginPackage[BEGIN_PACKAGE_LENGTH] = {'M','N','B','G', SESSION_ID, LSB0((uint16_t)TOTAL_PACKAGES_NUMBER), LSB1((uint16_t)TOTAL_PACKAGES_NUMBER)};
uint8_t arrayMainPackage[MAIN_PACKAGE_LENGTH]   = {'M','N','M','A', 'M','N','M','A', 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0xCC};
uint8_t arrayEndPackage[END_PACKAGE_LENGTH]     = {'M','N','E','N', 0xCC};

void setup()
{
    Serial.begin(9600);

    while(!Serial);

    if(!emitterObject.init()) //Checar se houve a inicializacao correta do emissor
    {
        Serial.println("EMISSOR INIT FAILED!");
    }

    else
        Serial.println("System initialized!");

    emitterObject.setFrequency(TELEMETRY_FREQUENCY);
}


void loop()
{

    if (!EndPackageSent)
    {
        // Send Begin Package
        if (!sessionSaved)
        {
            arrayBeginPackage[BEGIN_PACKAGE_INDEX_CRC] = crc8(arrayBeginPackage, BEGIN_PACKAGE_LENGTH - 1);

            // Send session while receiver dont save
            Serial.print("Sending Begin Package...");
            emitterObject.send(arrayBeginPackage , BEGIN_PACKAGE_LENGTH);
            Serial.println(" Sent!");

            emitterObject.waitPacketSent();
        }

        // Send Main Package
        else if (currentNumberPackagesSent < TOTAL_PACKAGES_NUMBER)
        {
            uint8_t a = crc8(arrayMainPackage,  MAIN_PACKAGE_LENGTH - 1);
            arrayMainPackage[MAIN_PACKAGE_INDEX_CRC] = a;

            arrayMainPackage[MAIN_PACKAGE_INDEX_ID_PACKAGE_LSB0] = LSB0(currentNumberPackagesSent);
            arrayMainPackage[MAIN_PACKAGE_INDEX_ID_PACKAGE_LSB1] = LSB1(currentNumberPackagesSent);
            
            Serial.print("Sending Main Package...");
            emitterObject.send(arrayMainPackage, MAIN_PACKAGE_LENGTH);
            Serial.println(" Sent!");

            Serial.print("main package crc: ");
            Serial.print(arrayMainPackage[MAIN_PACKAGE_INDEX_CRC]);

            Serial.print("crc8: ");
            Serial.println(a);

            Serial.println(currentNumberPackagesSent);
            currentNumberPackagesSent++;
        }

        // Send End Package
        else
        {
            arrayEndPackage[END_PACKAGE_INDEX_CRC] = crc8(arrayEndPackage, END_PACKAGE_LENGTH - 1);

            Serial.print("Sending End Package...");
            emitterObject.send(arrayEndPackage, END_PACKAGE_LENGTH);
            Serial.println(" Sent!");
        }

        // MENSAGEM DE VOLTA:
        if (emitterObject.waitAvailableTimeout(3000))
        {
            if (emitterObject.recv(bufReceived, &lenBuf))
            {
                Serial.print("Response Message: ");
                Serial.println((char*)bufReceived);

                if (!memcmp(bufReceived, arrayComeBackReceived, sizeof(arrayComeBackReceived))) // when trasmitter receive that message, he will
                {                                                  // set sessionSaved to true 
                    sessionSaved = true;
                }else if (!memcmp(bufReceived, arraytoSignEndPackageReceived, sizeof(arraytoSignEndPackageReceived)))
                {
                    EndPackageSent = true;
                }
            }
                                    // id package + 1
        }

    }

}