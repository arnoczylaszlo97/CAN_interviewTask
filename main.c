#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//---------------------------------------------TASK---------------------------------------------

/*
 * Irjon egy rovid programot, amely a jarmu szabvanyos OBD portjan keresztul lekerdezi masodpercenkent a motorfordulatszamot,
 * a jarmu sebesseget, es a hutokozeg homersekletet. Ha jaro motornal a homerseklet nagyobb mint 100C, hagyja abba a lekerdezest es irjon egy
 * hibauzenetet a consolra (printf)
 *
 * Elerheto fuggvenyek:
 *
 * SendPacket:
 *		CAN csomag kuldesere szolgalo prototipus fuggveny (nem szukseges megirni, csak hivni kell szukseg eseten)
 *
 * PacketReceived
 *		Callback fuggveny, amely lefut, ha az eszkoz vesz egy can csomagot
 *
 * Task100msec
 *		100secenkent lefuto task.
 *
 * Egy kis segitseg:
 * https://en.wikipedia.org/wiki/OBD-II_PIDs
 **/

#define BYTE unsigned char
#define WORD unsigned short
#define U16 unsigned short
#define U32 unsigned int
#define U64 unsigned long long

//---------------------------------------------TASK---------------------------------------------

#define vechicleSpeed 0x0D
#define engineSpeed 0x0C
#define engineCoolant 0x05

BYTE flgEngineRun = 0;
BYTE flgCoolTemp = 0;
BYTE timer100sec = 0;

typedef struct
{
    U32 StdId; /*!< Specifies the standard identifier.
                          This parameter can be a value between 0 to 0x7FF. */

    U32 ExtId; /*!< Specifies the extended identifier.
                          This parameter can be a value between 0 to 0x1FFFFFFF. */

    BYTE IDE; /*!< Specifies the type of identifier for the message. This parameter can be a value of
                          0=Standard, 1=Extended */

    BYTE DLC; /*!< Specifies the length of the frame.
                          This parameter can be a value between 0 to 8 */

    BYTE Data[8]; /*!< Contains the data to the packet. It ranges from 0 to 0xFF. */

} canMsg;

void SendPacket(canMsg *packet);

U64 ByteHextoDec(canMsg *packet)
{
    U64 decimalValue = 0;
    for (int i = 0; i < packet->DLC; i++)
    {
        decimalValue |= ((U64)packet->Data[i] << (8 * (packet->DLC - 1 - i)));
    }
    return decimalValue;
}

/*
* standard id: 11 bit
* extended id: 29 bit
* both have the PIDs in the first 11 bit
* Id & 0x7FF = checks the first 11 bit
* Vehicle speed    = 13 -> 0x0D    0 - 255       -> uint16 (can be uint8 because of 255)
* Engine Speed     = 12 -> 0x0C    0 - 16,383.75 -> uint32(can be float but RPM usually integer)
* Engine Coolant   = 5  -> 0x05    -40 - 215     -> uint16 (int16_t?)
*/
void HandlePacket(canMsg *packet, U32 Id)
{
    switch(Id & 0x7FF)
    {
        case vechicleSpeed:
        {
            BYTE vSpeed = ByteHextoDec(packet);
            printf("Vehicle Speed: %u [km/h] \n", vSpeed);
            break;
        }
        case engineSpeed:
        {
            U32 eSpeed = ByteHextoDec(packet);
            printf("Engine Speed: %u [RPM] \n", eSpeed);
            if (eSpeed > 0)
            {
                flgEngineRun = 1;
            }
            else
            {
                flgEngineRun = 0;
            }
            break;
        }
        case engineCoolant:
        {
            U16 tCoolant = ByteHextoDec(packet);
            printf("Coolant Temperature: %u [°C] \n", tCoolant);
            if (tCoolant >= 100)
            {
                flgCoolTemp = 1;
            }
            else
            {
                flgCoolTemp = 0;
            }
            break;
        }
        default:
            printf("Unknown packet ID: %u\n", Id);
    }
}

void PacketReceived(canMsg *packet)
{
    if (packet->IDE == 0)
    {
        HandlePacket(packet, packet->StdId);
    }
    else
    {
        HandlePacket(packet, packet->ExtId);
    }
}

// 100 * x = 100.000 (x=1000)
void Task100msec(void)
{
    if (timer100sec >= 1000)
    {
        if (flgCoolTemp == 1 && flgEngineRun == 1)
        {
            printf("Error: Coolant temperature exceeds 100°C. Stopping queries.\n");
        }
        else
        {
            timer100sec = 0;
            canMsg packet = {0};
            SendPacket(&packet);
        }
    }
    else
    {
        timer100sec++;
    }
}

//-----------------------------------TESTING--------------------------------------
int main()
{
    /*
    * Test Data:
    * 100 (dec) -> 0x64  -> Bit 0 = 0x64 (hex)
    * 1023(dec) -> 0x3ff -> Bit 0 = 0x03, Bit 1 = 0xff (hex)
    * Vehicle speed    = 13 -> 0x0D
    * Engine Speed     = 12 -> 0x0C
    * Engine Coolant   = 5  -> 0x05
    */

    canMsg packet;

    packet.StdId = 0x05;
    packet.ExtId = 0x0D;
    packet.IDE = 0x00;
     for (int i = 0; i < 8; i++)
    {
        packet.Data[i] = 0x00;
    }

    packet.Data[0] = 0x03;
    packet.Data[1] = 0xff;
    packet.DLC = 0x01;

    printf("CAN message parameters:\n");
    printf("->Standard ID: %u\n", packet.StdId);
    printf("->Extended ID: %u\n", packet.ExtId);
    printf("->IDE: %u\n", packet.IDE);
    printf("->DLC: %u\n", packet.DLC);
    printf("->Data: ");
    printf("\n");

    for (int i = 0; i < 8; i++)
    {
        printf("0x%02X ", packet.Data[i]);
    }
    printf("\n\n");

    PacketReceived(&packet);
    printf("flgEngineRun flag : %u\n", flgEngineRun);
    printf("flgCoolTemp flag : %u\n", flgCoolTemp);
    return 0;
}
//-----------------------------------TESTING--------------------------------------
