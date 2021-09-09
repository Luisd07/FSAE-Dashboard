/*
 * Florida International University
 * Formula SAE Dash Firmware 2.0
 * By: Iran E. Garcia
 * c 2019 FIU FSAE
 */

#include <mcp_can.h>
#include <stdint.h>

//#define DASH_DEBUG

/* Define ECU CAN PGN */
#define PGN_TPS   0x18F00300 //61443 Bits 2&3; 8  bits; 
#define PGN_RPM   0x18F00400 //61444 Bits 4-5; 16 bits; 0.125 resolution;
#define PGN_HOURS 0x18FEE500 //65253 Bits 1-4; 32 bits; 0.050 resolution;
#define PGN_TEMPS 0x18FEEE00 //65262
#define PGN_PRESS 0x18FEEF00 //65263
#define PGN_SPEED 0x18FEF100 //65265 Unused yet.
#define PGN_MAP   0x18FEF600 //65270
#define PGN_BATT  0x18FEF700 //65271
#define PGN_TANS  0x18FEF800 //65272 Not usable. No tansmission sensors

/* Define Dash Mainboard Pinout*/

#define CS_PIN 3

MCP_CAN CAN(CS_PIN);

uint8_t len = 0;
uint8_t buf[8];
uint32_t canID;

uint16_t ECU_RPM;//
uint8_t ECU_T_COOLANT;//
uint16_t ECU_T_OIL;//
uint16_t ECU_VBAT;
uint8_t ECU_TPS;//
uint8_t ECU_E_LOAD;//
uint32_t ECU_E_HOURS;
uint8_t ECU_MAP_T;//
uint8_t ECU_MAP_P;//

int RPM;
int T_COOLANT;
int T_OIL;
float MAP_T;
float MAP_P;
float VBAT;
float TPS;
float E_HOURS;
bool engine_on = false;
bool repeat = false;

void setup() {
  
  Serial.begin(115200);

  while (CAN_OK != CAN.begin(CAN_1000KBPS, MCP_8MHz)) //MCP_8MHz for other CAN module
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    
    delay(100);
  }
  Serial.println("CAN Bus Shield Init OK!");
}

void loop() {
  
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    
    CAN.readMsgBuf(&len, buf);
    canID = CAN.getCanId();

    #ifdef DASH_DEBUG
      Serial.print("Can ID:0x");
      Serial.println(canID, HEX);
    #endif
    
    /* Get RPM Data: SPN 160 J1939-71*/
    if(canID == PGN_RPM) {
      ECU_RPM = (buf[4] << 8) | buf[3];
      RPM = ECU_RPM * 0.125;

      #ifdef DASH_DEBUG 
        Serial.print("RPM: ");
        Serial.println(RPM);

        for(int i = 0; i<len; i++)    // print the data
        {
            Serial.print(buf[i], HEX);
            Serial.print("\t");
        }
        Serial.println();
      #endif

      #ifndef DASH_DEBUG
        Serial.print("rpm.val=");
        Serial.print(RPM);
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);

        if(RPM > 0) {
          Serial.print("vis e_light,0");
          Serial.write(0xff);
          Serial.write(0xff);
          Serial.write(0xff);
        }
        if(RPM == 0){
          Serial.print("vis e_light,1");
          Serial.write(0xff);
          Serial.write(0xff);
          Serial.write(0xff);
        }
        
      #endif
    }

    /* Get TPS Data: SPN 91 J1939-71*/
    if(canID == PGN_TPS) {
      
      ECU_TPS = buf[1];
      TPS = ECU_TPS * 0.4;

      ECU_E_LOAD = buf[2];
      
      #ifdef DASH_DEBUG
        Serial.println("PGN_TPS:");
        Serial.print("TPS Pos: ");
        Serial.print(TPS);
        Serial.println("%");
        
        Serial.print("Engine Load: ");
        Serial.print(ECU_E_LOAD);
        Serial.println("%");
        
        for(int i = 0; i < len; i++)    // print the data
        {
            Serial.print(buf[i], HEX);
            Serial.print("\t");
        }
        Serial.println();
      #endif

      #ifndef DASH_DEBUG
        Serial.print("tps_bar.val=");
        Serial.print((int)TPS);
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      #endif
    }

    /* Get Engine Hours: SPN 247 J1939-71*/
    if(canID == PGN_HOURS) {
      
      ECU_E_HOURS = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]; // This will give you an warning. Ignore it, compiler is dumb.
      E_HOURS = ECU_E_HOURS * 0.05; 
      
      #ifdef DASH_DEBUG
        Serial.print("PGN_HOURS: ");
        Serial.print(E_HOURS);
        Serial.println("hrs");
        for(int i = 0; i < len; i++)    // print the data
        {
            Serial.print(buf[i], HEX);
            Serial.print("\t");
        }
        Serial.println();
      #endif
    }

    /* Get MAP Data: SPN 247 J1939-71*/
    if(canID == PGN_MAP) {

      ECU_MAP_T = buf[2];
      MAP_T = ((ECU_MAP_T - 40) * 1.8) + 32; // Because 'Merica.
      
      ECU_MAP_P = buf[3];
      MAP_P = (ECU_MAP_P * 2) / 6.895; // Erick likes Bars
        
      #ifdef DASH_DEBUG
        Serial.println("PGN_MAP: ");
        Serial.print("MAP Temp: ");
        Serial.print(MAP_T);
        Serial.println("F");
        
        Serial.print("MAP Pres: ");
        Serial.print(MAP_P);
        Serial.println("psi");

        for(int i = 0; i < len; i++)    // print the data
        {
            Serial.print(buf[i], HEX);
            Serial.print("\t");
        }
        Serial.println();
      #endif

      #ifndef DASH_DEBUG
        Serial.print("o_pres.txt=\"");
        Serial.print(MAP_P);
        Serial.print("\"");
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      #endif
    }

    /* Get Temperature Data: SPN 110 J1939-71*/
    if(canID == PGN_TEMPS) {
      ECU_T_COOLANT = buf[0];
      T_COOLANT = ((ECU_T_COOLANT - 40) * 1.8) + 32;

      #ifdef DASH_DEBUG
        Serial.print("Coolant Temp: ");
        Serial.println(T_COOLANT);
      #endif

      #ifndef DASH_DEBUG
        Serial.print("e_temp.txt=\"");
        Serial.print(T_COOLANT);
        Serial.print("\"");
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      #endif

      ECU_T_OIL = (buf[3] << 8) | buf[2];
      T_OIL = (((ECU_T_OIL * 0.03125) - 273) * 1.8) + 32;

      #ifdef DASH_DEBUG
        Serial.print("Oil Temp: ");
        Serial.println(T_OIL);
      #endif

      #ifdef DASH_DEBUG
        for(int i = 0; i < len; i++) // print the data
        {
              Serial.print(buf[i], HEX);
              Serial.print("\t");
        }
        Serial.println();
      #endif

      #ifndef DASH_DEBUG
        Serial.print("o_temp.txt=\"");
        Serial.print(T_OIL);
        Serial.print("\"");
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      #endif
    }

    /* Get Battery Level: SPN 158 J1939-71*/
    if(canID == PGN_BATT) {
      ECU_VBAT = (buf[5] << 8) | buf[4];
      VBAT = ECU_VBAT * 0.05;

      #ifdef DASH_DEBUG
        Serial.print("VBAT: ");
        Serial.println(VBAT);

        for(int i = 0; i < len; i++)    // print the data
        {
            Serial.print(buf[i], HEX);
            Serial.print("\t");
        }
        Serial.println();
      #endif

      #ifndef DASH_DEBUG
        Serial.print("v_bat.txt=\"");
        Serial.print(VBAT);
        Serial.print("\"");
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
      #endif
    }

    #ifdef DASH_DEBUG
    Serial.println("==================================");
    #endif 
  }
}

