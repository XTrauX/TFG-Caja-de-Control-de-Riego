
/**
 * @file multirriego.cpp
 * @brief Implementation of multi-group irrigation control functions.
 *
 * This file contains the implementation of functions to manage and control 
 * multiple irrigation groups. It includes functions to get the status of 
 * multi-groups, set multi-group configurations by ID, display group information 
 * on LEDs, and print multi-group configurations for debugging purposes.
 *
 * The functions provided are:
 * - uint16_t getMultiStatus(): Returns the status of the multi-group based on button states.
 * - int setMultibyId(uint16_t id, Config_parm &cfg): Sets the multi-group configuration based on the given ID.
 * - void displayGrupo(uint16_t *serie, int serieSize): Displays the group information using LEDs.
 * - void printMultiGroup(Config_parm &cfg, int pgrupo): Prints the configuration of the specified group for debugging.
 *
 * @note This file is part of the ControlRiego-2.5 project.
 * @note Ensure TRACE and EXTRADEBUG macros are defined for additional debugging information.
 *
 * @version 2.5
 * @date 2024
 * @author Tomas
 */
#include "Control.h"

uint16_t getMultiStatus()
{
  if (Boton[bID_bIndex(bGRUPO1)].estado) return bGRUPO1;
  if (Boton[bID_bIndex(bGRUPO3)].estado) return bGRUPO3;
  return bGRUPO2  ;
}

int setMultibyId(uint16_t id, Config_parm &cfg)
{
  #ifdef TRACE
    Serial.printf("TRACE: in setMultibyId - recibe id=x%x \n", id);
  #endif
  for(int i=0; i<NUMGRUPOS; i++)
  {
    if(cfg.groupConfig[i].id == id) {
      multi.id = &cfg.groupConfig[i].id;
      multi.size = &cfg.groupConfig[i].size;
      multi.zserie = &cfg.groupConfig[i].serie[0];
      multi.desc = cfg.groupConfig[i].desc;
      for (int j=0; j < *multi.size; j++) {
        multi.serie[j] = ZONAS[cfg.groupConfig[i].serie[j]-1];  
        #ifdef EXTRADEBUG 
          Serial.printf("  Zona%d   id: x", cfg.groupConfig[i].serie[j]);
          Serial.println(Boton[cfg.groupConfig[i].serie[j]-1].id,HEX); 
        #endif  
      }
      return i+1;
    }
  }
  Serial.println(F("[ERROR] setMultibyID devuelve -not found-"));
  return 0;
}

void displayGrupo(uint16_t *serie, int serieSize)
{
  led(Boton[bID_bIndex(*multi.id)].led,ON);
  int i;
  for(i=0;i<serieSize;i++) {
    led(Boton[bID_bIndex(serie[i])].led,ON);
    delay(300);
    bip(i+1);
    led(Boton[bID_bIndex(serie[i])].led,OFF);
    delay(100);
  }
  led(Boton[bID_bIndex(*multi.id)].led,OFF);
}

void printMultiGroup(Config_parm &cfg, int pgrupo)
{
  for(int j = 0; j < cfg.groupConfig[pgrupo].size; j++) {
    Serial.printf("  Zona%d   id: x", cfg.groupConfig[pgrupo].serie[j]);
    Serial.println(Boton[cfg.groupConfig[pgrupo].serie[j]-1].id,HEX);
  }
  Serial.println();
}

