
/**
 * @file Control.cpp
 * @brief Implementation of the irrigation control system.
 * 
 * This file contains the main setup and loop functions for the irrigation control system,
 * as well as various helper functions to handle button presses, state transitions, and 
 * communication with the Domoticz home automation system.
 * 
 * The system supports different modes such as RELEASE, DEVELOP, and DEMO, which can be 
 * configured using preprocessor directives. It also includes debugging and tracing 
 * capabilities to aid in development and troubleshooting.
 * 
 * The main functionalities include:
 * - Initializing hardware components such as display, encoder, and various peripherals.
 * - Handling button presses to control the irrigation system.
 * - Managing different states of the system such as CONFIGURING, ERROR, REGANDO (watering), 
 *   TERMINANDO (finishing), STANDBY, STOP, and PAUSE.
 * - Communicating with the Domoticz system to get irrigation factors and control the irrigation zones.
 * - Periodic verification of system status and handling errors.
 * 
 * The system is designed to be flexible and configurable, with support for saving and loading 
 * configuration parameters, handling multiple irrigation zones, and providing feedback through 
 * LEDs and a display.
 * 
 * @note This file is part of the ControlRiego-2.5 project.
 * 
 * @version 2.5
 * @date Built on 2024
 * 
 * @author Tomas
 */
#define __MAIN__
#include "Control.h"

/*----------------------------------------------*
 *               Setup inicial                  *
 *----------------------------------------------*/
void setup()
{
  #ifdef RELEASE
                NONETWORK=false; 
                VERIFY=true; 
  #endif
  #ifdef DEVELOP
                NONETWORK=false;
                VERIFY=true; 
  #endif
  #ifdef DEMO
                NONETWORK=true;
                VERIFY=false;
  #endif

  Serial.begin(115200);
  Serial.println("\n\n CONTROL RIEGO V" + String(VERSION) + "    Built on " __DATE__ " at " __TIME__ );
  Serial.print(F("Startup reason: "));Serial.println(ESP.getResetReason());
  #ifdef TRACE
    Serial.println(F("TRACE: in setup"));
  #endif
  #ifdef DEBUG
   Serial.println(F("Inicializando display"));
  #endif
  display = new Display(DISPCLK,DISPDIO);
  display->clearDisplay();
  #ifdef DEBUG
   Serial.println(F("Inicializando Encoder"));
  #endif
  Encoder = new ClickEncoder(ENCCLK,ENCDT,ENCSW);
  #ifdef DEBUG
   Serial.println(F("Inicializando Configure"));
  #endif
  configure = new Configure(display);
  initCD4021B();
  initHC595();
  setupInit();
  led(LEDR,ON);
  #ifdef EXTRADEBUG
    printFile(parmFile);
  #endif
  setupParm();
  check();
  setupRedWM(config);
  if (saveConfig) {
    if (saveConfigFile(parmFile, config))  bipOK(3);;
    saveConfig = false;
  }
  delay(2000);
  timeClient.begin();
  delay(500);
  initClock();
  initLastRiegos();
  initFactorRiegos();
  Boton[bID_bIndex(bPAUSE)].flags.holddisabled = true;
  parseInputs(CLEAR);
  setupEstado();
  #ifdef EXTRADEBUG
    printMulti();
    printFile(parmFile);
  #endif
  tic_verificaciones.attach_scheduled(VERIFY_INTERVAL, flagVerificaciones);
  standbyTime = millis();
  #ifdef TRACE
    Serial.println(F("TRACE: ending setup"));
  #endif
}


/*----------------------------------------------*
 *            Bucle principal                   *
 *----------------------------------------------*/

void loop()
{
  #ifdef EXTRATRACE
    Serial.print(F("L"));
  #endif

  procesaBotones();
  dimmerLeds();
  procesaEstados();
  dimmerLeds();
  Verificaciones();
}

  /*----------------------------------------------*
   *                 Funciones                    *
   *----------------------------------------------*/


void procesaBotones()
{
  #ifdef EXTRATRACE
    Serial.print(F("B"));
  #endif
  (testButton(bENCODER, OFF)) ? encoderSW = true : encoderSW = false;
  if (multiSemaforo) multiSemaforo = false;
  else  boton = parseInputs(READ); 
  if(boton == NULL) return;
  if (reposo && boton->id != bSTOP) {
    Serial.println(F("Salimos de reposo"));
    reposo = false;
    displayOff = false;
    standbyTime = millis();
    if(Estado.estado == STOP) display->print("StoP");
    else StaticTimeUpdate();
    return;
  }
  if(Estado.estado == ERROR) return;
  if (!boton->flags.action) return;
  switch (boton->id) {
    case bPAUSE:
      procesaBotonPause();
      break;
    case bSTOP:
      procesaBotonStop();
      break;
    case bMULTIRIEGO:
      if (!procesaBotonMultiriego()) break;
    default:
      procesaBotonZona();
  }
}

void procesaEstados()
{
  #ifdef EXTRATRACE
    Serial.print(F("E"));
  #endif

  switch (Estado.estado) {
    case CONFIGURANDO:
      procesaEstadoConfigurando();
      break;
    case ERROR:
      procesaEstadoError();
      blinkPause();
      break;
    case REGANDO:
      procesaEstadoRegando();
      break;
    case TERMINANDO:
      procesaEstadoTerminando();
      break;
    case STANDBY:
      procesaEstadoStandby();
      break;
    case STOP:
      procesaEstadoStop();
      break;
    case PAUSE:
      procesaEstadoPause();
      strcmp(errorText, "") == 0 ? blinkPause() : blinkPauseError();
      break;
  }
}

void setupEstado() 
{
  #ifdef TRACE
    Serial.println(F("TRACE: in setupEstado"));
  #endif
  if(!setMultibyId(getMultiStatus(), config) || !config.initialized) {
    statusError(E0, 3); 
  return;
  }
  if (NONETWORK) {
    if (Boton[bID_bIndex(bSTOP)].estado) {
      setEstado(STOP);
      infoDisplay("StoP", NOBLINK, LONGBIP, 1);
    }
    else setEstado(STANDBY);
    bip(2);
    return;
  }
  if (Estado.estado == ERROR) {
    return;
  }
  if (checkWifi()) {
    if (Boton[bID_bIndex(bSTOP)].estado) {
      setEstado(STOP);
      infoDisplay("StoP", NOBLINK, LONGBIP, 1);
    }
    else setEstado(STANDBY);
    bip(1);
    return;
  }
  statusError(E1, 3);
}

void setupInit(void) {
  #ifdef TRACE
    Serial.println(F("TRACE: in setupInit"));
  #endif
  if (testButton(bENCODER, OFF)) {
    if (testButton(bGRUPO1,ON)) {
      initFlags.initParm = true;
      Serial.println(F("encoderSW pulsado y multirriego en GRUPO1  --> flag de load default PARM true"));
      loadDefaultSignal(6);
    }
    if (testButton(bGRUPO3,ON)) {
      initFlags.initWifi = true;
      Serial.println(F("encoderSW pulsado y multirriego en GRUPO3  --> flag de init WIFI true"));
      wifiClearSignal(6);
    }
  }
};


void procesaBotonPause(void)
{
  if (Estado.estado != STOP) {
    if(!boton->estado) return;
    switch (Estado.estado) {
      case REGANDO:
        if(encoderSW) {  
          setEstado(TERMINANDO);
          Serial.println(F("encoderSW+PAUSE terminamos riego de zona en curso"));
        }
        else {
          bip(1);
          setEstado(PAUSE);
          tic_parpadeoLedZona.detach(); 
          led(ultimoBoton->led,ON);
          stopRiego(ultimoBoton->id);
          T.PauseTimer();
        }
        break;
      case PAUSE:
        if(simular.ErrorPause) statusError(E2,3); 
        else initRiego(ultimoBoton->id);
        if(Estado.estado == ERROR) { 
          ledID = ultimoBoton->led;
          tic_parpadeoLedZona.attach(0.2,parpadeoLedZona);
          Serial.printf( "error al salir de PAUSE errorText : %s Estado.fase : %d\n", errorText, Estado.fase );
          refreshTime();
          setEstado(PAUSE);
          break;
        }
        bip(2);
        T.ResumeTimer();
        tic_parpadeoLedZona.detach(); 
        led(ultimoBoton->led,ON);
        setEstado(REGANDO);
        break;
      case STANDBY:
          boton = NULL; 
          if(encoderSW) {  
            if (NONETWORK) {
                NONETWORK = false;
                Serial.println(F("encoderSW+PAUSE pasamos a modo NORMAL y leemos factor riegos"));
                bip(2);
                led(LEDB,OFF);
                display->print("----");
                initFactorRiegos();
                if(VERIFY && Estado.estado != ERROR) stopAllRiego(); 
            }
            else {
                NONETWORK = true;
                Serial.println(F("encoderSW+PAUSE pasamos a modo NONETWORK (DEMO)"));
                bip(2);
                led(LEDB,ON);
            }
          }
          else {  
            ultimosRiegos(SHOW);
            delay(3000);
            ultimosRiegos(HIDE);
          }
          standbyTime = millis();
    }
  }
  else {
    if(boton->estado) {
      if(!holdPause) {
        countHoldPause = millis();
        holdPause = true;
      }
      else {
        if((millis() - countHoldPause) > HOLDTIME) {
          if(!encoderSW) { 
            configure->start();
            longbip(1);
            ledConf(ON);
            setEstado(CONFIGURANDO);
            Serial.println(F("Stop + hold PAUSA --> modo ConF()"));
            boton = NULL;
            holdPause = false;
            savedValue = value;
          }
          else {  
            Serial.println(F("Stop + encoderSW + PAUSA --> Reset....."));
            longbip(3);
            ESP.restart();  
          }
        }
      }
    }
    else holdPause = false;
  }
};


void procesaBotonStop(void)
{
  if (boton->estado) {  
    if (Estado.estado == REGANDO || Estado.estado == PAUSE) {
      
      display->print("StoP");
      T.StopTimer();
      if (!stopAllRiego()) {   
        boton = NULL;
        return; 
      }
      infoDisplay("StoP", DEFAULTBLINK, BIP, 6);;
      setEstado(STOP);
      resetFlags();
    }
    else {
      infoDisplay("StoP", NOBLINK, BIP, 3);
      if (!stopAllRiego()) {  
        boton = NULL;
        return; 
      }
      setEstado(STOP);
      reposo = true;
      displayOff = false;
    }
  }
  if (!boton->estado && Estado.estado == STOP) {
    StaticTimeUpdate();
    reposo = false; 
    displayOff = false;
    setEstado(STANDBY);
  }
  standbyTime = millis();
}


bool procesaBotonMultiriego(void)
{
  if (Estado.estado == STANDBY && !multirriego) {
    int n_grupo = setMultibyId(getMultiStatus(), config);
    if (n_grupo == 0) { 
      statusError(E0, 3); 
      return false;
    }  
    #ifdef DEBUG
      Serial.printf( "en MULTIRRIEGO, setMultibyId devuelve: Grupo%d (%s) multi.size=%d \n" , n_grupo, multi.desc, *multi.size);
      for (int k=0; k < *multi.size; k++) Serial.printf( "       multi.serie: x%x \n" , multi.serie[k]);
      Serial.printf( "en MULTIRRIEGO, encoderSW status  : %d \n", encoderSW );
    #endif
    if (encoderSW) {
      char version_n[10];
      strncpy(version_n, VERSION, 10); 
      std::remove(std::begin(version_n),std::end(version_n),'.');
      std::remove(std::begin(version_n),std::end(version_n),'-');
      display->print(version_n);
      displayGrupo(multi.serie, *multi.size);
      #ifdef DEBUG
        Serial.printf( "en MULTIRRIEGO + encoderSW, display de grupo: %s tamaño: %d \n", multi.desc , *multi.size );
      #endif
      StaticTimeUpdate();
      return false;           
    }  
    else {
      bip(4);
      multirriego = true;
      multi.actual = 0;
      Serial.printf("MULTIRRIEGO iniciado: %s \n", multi.desc);
      led(Boton[bID_bIndex(*multi.id)].led,ON);
      boton = &Boton[bID_bIndex(multi.serie[multi.actual])];
    }
  }
  return true;   
}


void procesaBotonZona(void)
{
  int zIndex = bID_zIndex(boton->id);
  if (zIndex == 999) return; 
  int bIndex = bID_bIndex(boton->id); 
  if (Estado.estado == STANDBY) {
    if (!encoderSW || multirriego) {  
        bip(2);
        uint8_t fminutes=0,fseconds=0;
        if(multirriego) {
          timeByFactor(factorRiegos[zIndex],&fminutes,&fseconds);
        }
        else {
          fminutes = minutes;
          fseconds = seconds;
        }
        #ifdef DEBUG
          Serial.printf("Minutos: %d Segundos: %d FMinutos: %d FSegundos: %d\n",minutes,seconds,fminutes,fseconds);
        #endif
        ultimoBoton = boton;
        if ((fminutes == 0 && fseconds == 0) || boton->idx == 0) {
          setEstado(TERMINANDO);
          led(Boton[bIndex].led,ON); 
          display->print("-00-");
          return;
        }
        T.SetTimer(0,fminutes,fseconds);
        T.StartTimer();
        initRiego(boton->id);
        if(Estado.estado != ERROR) setEstado(REGANDO); 
    }
    else { 
      led(Boton[bIndex].led,ON);
      #ifdef DEBUG
        Serial.printf("Boton: %s Factor de riego: %d \n", boton->desc,factorRiegos[zIndex]);
        Serial.printf("          boton.index: %d \n", bIndex);
        Serial.printf("          boton(%d).led: %d \n", bIndex, Boton[bIndex].led);
      #endif
      savedValue = value;
      value = factorRiegos[zIndex];
      display->print(value);
      delay(2000);
      value = savedValue; 
      led(Boton[bIndex].led,OFF);
      StaticTimeUpdate();
    }
  }
}


void procesaEstadoConfigurando()
{
  Boton[bID_bIndex(bPAUSE)].flags.holddisabled = true;
  if (boton != NULL) {
    if (boton->flags.action) {
      int n_grupo;
      int bIndex = bID_bIndex(boton->id);
      int zIndex = bID_zIndex(boton->id);
      switch(boton->id) {
        case bMULTIRIEGO:
          if (configure->configuring()) return; 
          n_grupo = setMultibyId(getMultiStatus(), config);
          if (encoderSW) { 
            if (n_grupo == 1) {  
              if (copyConfigFile(parmFile, defaultFile)) {
                Serial.println(F("[ConF] salvado fichero de parametros actuales como DEFAULT"));
                infoDisplay("-dEF", DEFAULTBLINK, BIPOK, 5);
                display->print("ConF"); 
              }
            }
            #ifdef WEBSERVER
              if (n_grupo == 2) {  
                setupWS();
                Serial.println(F("[ConF][WS] activado webserver para actualizaciones OTA de SW o filesystem"));
                webServerAct = true;
                ledConf(OFF);
                infoDisplay("otA", DEFAULTBLINK, BIPOK, 5);
              }
            #endif 
            if (n_grupo == 3) { 
              Serial.println(F("[ConF] encoderSW + selector ABAJO: activamos AP y portal de configuracion"));
              ledConf(OFF);
              starConfigPortal(config);
              ledConf(ON);
              display->print("ConF");
            }
          }
          else {
            configure->configureMulti(n_grupo);
            Serial.printf( "[ConF] configurando: GRUPO%d (%s) \n" , n_grupo, multi.desc);
            #ifdef DEBUG
              Serial.printf( "en configuracion de MULTIRRIEGO, setMultibyId devuelve: Grupo%d (%s) multi.size=%d \n" , n_grupo, multi.desc, *multi.size);
            #endif            
            displayGrupo(multi.serie, *multi.size);
            multi.w_size = 0 ; 
            display->print("PUSH");
            led(Boton[bID_bIndex(*multi.id)].led,ON);
          } 
          break;
        case bPAUSE:
          if(!boton->estado) return; 
          if(!configure->configuring()) {
            configure->configureTime();   
            Serial.println(F("[ConF] configurando tiempo riego por defecto"));
            delay(500);
            break;
          }
          if(configure->configuringTime()) {
            Serial.printf( "[ConF] Save DEFAULT TIME, minutes: %d  secons: %d \n", minutes, seconds);
            config.minutes = minutes;
            config.seconds = seconds;
            saveConfig = true;
            bipOK(3);
            configure->stop();
            break;
          }
          if(configure->configuringIdx()) {
            int bIndex = configure->getActualIdxIndex();
            int zIndex = bID_zIndex(Boton[bIndex].id);
            Boton[bIndex].idx = (uint16_t)value;
            config.botonConfig[zIndex].idx = (uint16_t)value;
            saveConfig = true;
            Serial.printf( "[ConF] Save Zona%d (%s) IDX value: %d \n", zIndex+1, Boton[bIndex].desc, value);
            value = savedValue;
            bipOK(3);
            led(Boton[bIndex].led,OFF);
            configure->stop();
            break;
          }
          if(configure->configuringMulti()) {
            if (multi.w_size) { 
              *multi.size = multi.w_size;
              int g = configure->getActualGrupo();
              for (int i=0; i<multi.w_size; ++i) {
                config.groupConfig[g-1].serie[i] = bID_zIndex(multi.serie[i])+1;
              }
              Serial.printf( "[ConF] SAVE PARM Multi : GRUPO%d  tamaño: %d (%s)\n", g , *multi.size , multi.desc );
              printMultiGroup(config, g-1);
              saveConfig = true;
              bipOK(3);
            }
            ultimosRiegos(HIDE);
            led(Boton[bID_bIndex(*multi.id)].led,OFF);
            configure->stop();
          }
          break;
        case bSTOP:
          if(!boton->estado) { 
            configure->stop();
            if (saveConfig) {
              Serial.println(F("saveConfig=true  --> salvando parametros a fichero"));
              if (saveConfigFile(parmFile, config)) infoDisplay("SAUE", DEFAULTBLINK, BIPOK, 5);
              saveConfig = false;
            }
            #ifdef WEBSERVER
              if (webServerAct) {
                endWS();
                Serial.println(F("[ConF][WS] desactivado webserver"));
                webServerAct = false; 
              }
            #endif
            setEstado(STANDBY);
            resetLeds();
            standbyTime = millis();
            if (savedValue>0) value = savedValue;  
            StaticTimeUpdate();
          }
          break;
        default: 
          if (configure->configuringMulti()) {  
            if (multi.w_size < 16) { 
              multi.serie[multi.w_size] = boton->id;
              Serial.printf("[ConF] añadiendo ZONA%d (%s) \n",zIndex+1, boton->desc);
              multi.w_size = multi.w_size + 1;
              led(Boton[bIndex].led,ON);
            }
            else longbip(1);
          }
          if (!configure->configuring()) {  
            Serial.printf("[ConF] configurando IDX boton: %s \n",boton->desc);
            configure->configureIdx(bIndex);
            value = boton->idx;
            led(Boton[bIndex].led,ON);
          }
      }
    }
  }
  else procesaEncoder();
};


void procesaEstadoError(void)
{
  if(boton == NULL) return; 
  if(boton->id == bPAUSE && boton->estado) { 
    if (Boton[bID_bIndex(bSTOP)].estado) {
      setEstado(STOP);
      infoDisplay("StoP", NOBLINK, LONGBIP, 1);
      displayOff = true;
    }
    else {
      setEstado(STANDBY);
      displayOff = false;
      standbyTime = millis();
      StaticTimeUpdate();
    } 
    NONETWORK = true;
    Serial.println(F("estado en ERROR y PAUSA pulsada pasamos a modo NONETWORK y reseteamos"));
    bip(2);
    resetLeds(); 
    resetFlags();  
  }
  if(boton->id == bSTOP) {
    setEstado(STANDBY);
    if(checkWifi()) stopAllRiego();
    Serial.println(F("ERROR + STOP --> Reset....."));
    longbip(3);
    ESP.restart();  
  }
};


void procesaEstadoRegando(void)
{
  tiempoTerminado = T.Timer();
  if (T.TimeHasChanged()) refreshTime();
  if (tiempoTerminado == 0) setEstado(TERMINANDO);
  else if(flagV && VERIFY) { 
    if(queryStatus(ultimoBoton->idx, (char *)"On")) return;
    else {
      ledID = ultimoBoton->led;
      if(Estado.fase == CERO) { 
        bip(1);
        T.PauseTimer();
        tic_parpadeoLedZona.attach(0.8,parpadeoLedZona);
        Serial.printf(">>>>>>>>>> procesaEstadoRegando zona: %s en PAUSA remota <<<<<<<<\n", ultimoBoton->desc);
        setEstado(PAUSE);
      }
      else {
        statusError(Estado.fase, 3); 
        tic_parpadeoLedON.attach(0.2,parpadeoLedON);
        tic_parpadeoLedZona.attach(0.4,parpadeoLedZona);
        errorOFF = true;  
        Serial.println(F("[ERROR] procesaEstadoRegando: SE HA DEVUELTO ERROR"));
      }  
    }
  }
};


void procesaEstadoTerminando(void)
{
  bip(5);
  tic_parpadeoLedZona.detach(); 
  stopRiego(ultimoBoton->id);
  if (Estado.estado == ERROR) return; 
  display->blink(DEFAULTBLINK);
  led(Boton[bID_bIndex(ultimoBoton->id)].led,OFF);
  StaticTimeUpdate();
  standbyTime = millis();
  setEstado(STANDBY);
  if (multirriego) {
    multi.actual++;
    if (multi.actual < *multi.size) {
      boton = &Boton[bID_bIndex(multi.serie[multi.actual])];
      multiSemaforo = true;
    }
    else {
      bipEND(5);
      resetFlags();
      Serial.printf("MULTIRRIEGO %s terminado \n", multi.desc);
      led(Boton[bID_bIndex(*multi.id)].led,OFF);
    }
  }
};


void procesaEstadoStandby(void)
{
  Boton[bID_bIndex(bPAUSE)].flags.holddisabled = true;
  if (reposo) standbyTime = millis();
  else {
    if (millis() > standbyTime + (1000 * STANDBYSECS)) {
      Serial.println(F("Entramos en reposo"));
      reposo = true;
      display->clearDisplay();
    }
  }
  procesaEncoder();
};


void procesaEstadoStop(void)
{
  Boton[bID_bIndex(bPAUSE)].flags.holddisabled = false;
  if(reposo && !displayOff) {
    if (millis() > standbyTime + (4 * 1000 * STANDBYSECS) && reposo) {
      display->clearDisplay();
      displayOff = true;
    }
  }
};

void procesaEstadoPause(void) {
  if(flagV && VERIFY) {  
    if(queryStatus(ultimoBoton->idx, (char *)"Off")) return;
    else {
      if(Estado.fase == CERO) { 
        bip(2);
        ledID = ultimoBoton->led;
        Serial.printf("\tactivado blink %s (boton id= %d) \n", ultimoBoton->desc, ultimoBoton->id);
        tic_parpadeoLedZona.attach(0.8,parpadeoLedZona);
        Serial.printf(">>>>>>>>>> procesaEstadoPause zona: %s activada REMOTAMENTE <<<<<<<\n", ultimoBoton->desc);
        T.ResumeTimer();
        setEstado(REGANDO);
      }
      else Estado.fase = CERO; 
    }
  }
}

void setEstado(uint8_t estado)
{
  Estado.estado = estado;
  Estado.fase = CERO;
  strcpy(errorText, "");
  #ifdef DEBUG
    Serial.printf("setEstado Cambiado estado a: %s \n", nEstado[estado]);
  #endif
}

void check(void)
{
  display->print("----");
  #ifndef DEBUG
  initLeds();
  display->check(1);
  #endif
}

void initFactorRiegos()
{
  #ifdef TRACE
    Serial.println(F("TRACE: in initFactorRiegos"));
  #endif
  for(uint i=0;i<NUMZONAS;i++) {
    factorRiegos[i]=100;
  }
  for(uint i=0;i<NUMZONAS;i++) 
  {
    int bIndex = bID_bIndex(ZONAS[i]);
    uint factorR = getFactor(Boton[bIndex].idx);
    if(factorR == 999) break; 
    if(Estado.estado == ERROR) { 
      if(Estado.fase == E3) {    
        ledID = Boton[bIndex].led;
        tic_parpadeoLedZona.attach(0.4,parpadeoLedZona);
      }
      break;
    }
    factorRiegos[i] = factorR;
    if (strlen(descDomoticz)) {
      if (xNAME) {
        strlcpy(Boton[bIndex].desc, descDomoticz, sizeof(Boton[bIndex].desc));
        Serial.printf("\tdescripcion ZONA%d actualizada en boton \n", i+1);
      }
      if (config.botonConfig[i].desc[0] == 0) {
        strlcpy(config.botonConfig[i].desc, descDomoticz, sizeof(config.botonConfig[i].desc));
        strlcpy(Boton[bIndex].desc, descDomoticz, sizeof(Boton[bIndex].desc));
        Serial.printf("\tdescripcion ZONA%d incluida en config \n", i+1);
      }  
    }
  }
  #ifdef VERBOSE
    Serial.print(F("Factores de riego "));
    factorRiegosOK ? Serial.println(F("leidos: ")) :  Serial.println(F("(simulados): "));
    for(uint i=0;i<NUMZONAS;i++) {
      Serial.printf("\tfactor ZONA%d: %d (%s) \n", i+1, factorRiegos[i], Boton[bID_bIndex(ZONAS[i])].desc);
    }
  #endif
}


void timeByFactor(int factor,uint8_t *fminutes, uint8_t *fseconds)
{
  uint tseconds = (60*minutes) + seconds;
  tseconds = (tseconds*factor)/100;
  *fminutes = tseconds/60;
  *fseconds = tseconds%60;
}


void initClock()
{
  if (timeClient.update()) {
    setTime(timeClient.getEpochTime());
    timeOK = true;
    Serial.print("initClock: NTP time recibido OK  (UTC) --> " + timeClient.getFormattedTime());
    time_t t = CE.toLocal(now(),&tcr);
    Serial.printf("  local --> %d:%d:%d \n" ,hour(t),minute(t),second(t));
  }  
   else {
     Serial.println(F("[ERROR] initClock: no se ha recibido time por NTP"));
     timeOK = false;
    }
}


void ultimosRiegos(int modo)
{
  switch(modo) {
    case SHOW:
      time_t t;
      utc = timeClient.getEpochTime();
      t = CE.toLocal(utc,&tcr);
      for(uint i=0;i<NUMZONAS;i++) {
        if(lastRiegos[i] > previousMidnight(t)) {
            led(Boton[bID_bIndex(ZONAS[i])].led,ON);
        }
      }
      display->printTime(hour(t),minute(t));
      break;
    case HIDE:
      StaticTimeUpdate();
      for(unsigned int i=0;i<NUMZONAS;i++) {
        led(Boton[bID_bIndex(ZONAS[i])].led,OFF);
      }
      break;
  }
}


void dimmerLeds()
{
  if (reposo) { 
    led(LEDR,OFF);
    led(LEDG,OFF);
    led(LEDB,OFF);
    delay(1);
    led(LEDR,ON);
    if(connected) led(LEDG,ON);
    if(NONETWORK) led(LEDB,ON);
  }   
}

void procesaEncoder()
{
  #ifdef NODEMCU
    Encoder->service();
  #endif
  if(Estado.estado == CONFIGURANDO && configure->configuringIdx()) {
      #ifdef EXTRATRACE
       Serial.print(F("i"));
      #endif
      value -= Encoder->getValue();
      if (value > 1000) value = 1000;
      if (value <  1) value = 0; 
      display->print(value);
      return;
  }

  if (Estado.estado == CONFIGURANDO && !configure->configuringTime()) return;

  if(!reposo) StaticTimeUpdate();
  value -= Encoder->getValue();
  if(seconds == 0 && value>0) {
    if (value > MAXMINUTES)  value = MAXMINUTES;
    if (value != minutes) {
      minutes = value;
    } else return;
  }
  else {
    if(value<60 && value>=MINSECONDS) {
      if (value != seconds) {
        seconds = value;
      } else return;
    }
    else if (value >=60) {
      value = minutes = 1;
      seconds = 0;
    }
    else if(minutes == 1) {
      value = seconds = 59;
      minutes = 0;
    }
    else
    {
      value = seconds = MINSECONDS;
      minutes = 0;
    }
  }
  reposo = false;
  StaticTimeUpdate();
  standbyTime = millis();
}


void initLastRiegos()
{
  for(uint i=0;i<NUMZONAS;i++) {
   lastRiegos[i] = 0;
  }
}


bool initRiego(uint16_t id)
{
  int bIndex = bID_bIndex(id);
  #ifdef DEBUG
    Serial.printf("Boton: %s boton.index: %d \n", boton->desc, bIndex);
  #endif
  int zIndex = bID_zIndex(id);
  time_t t;
  if(zIndex == 999) return false;
  Serial.printf( "Iniciando riego: %s \n", Boton[bIndex].desc);
  led(Boton[bIndex].led,ON);
  utc = timeClient.getEpochTime();
  t = CE.toLocal(utc,&tcr);
  lastRiegos[zIndex] = t;
  return domoticzSwitch(Boton[bIndex].idx, (char *)"On", DEFAULT_SWITCH_RETRIES);
}

bool stopRiego(uint16_t id)
{
  int bIndex = bID_bIndex(id);
  ledID = Boton[bIndex].led;
  #ifdef DEBUG
  Serial.printf( "Terminando riego: %s \n", Boton[bIndex].desc);
  #endif
  domoticzSwitch(Boton[bIndex].idx, (char *)"Off", DEFAULT_SWITCH_RETRIES);
  if (Estado.estado != ERROR) Serial.printf( "Terminado OK riego: %s \n" , Boton[bIndex].desc );
  else {     
    if (!errorOFF) { 
      errorOFF = true;  
      tic_parpadeoLedON.attach(0.2,parpadeoLedON);
      tic_parpadeoLedZona.attach(0.4,parpadeoLedZona);
    } 
    return false;
  }
  return true;
}


void resetLeds()
{
  for(unsigned int j=0;j<NUMGRUPOS;j++) {
    led(Boton[bID_bIndex(GRUPOS[j])].led,OFF);
  }
  tic_parpadeoLedZona.detach();
  for(unsigned int i=0;i<NUMZONAS;i++) {
    led(Boton[bID_bIndex(ZONAS[i])].led,OFF);
  }
  tic_parpadeoLedON.detach();
  ledConf(OFF);
}


void resetFlags()
{
  multirriego = false;
  multiSemaforo = false;
  errorOFF = false;
  falloAP = false;
  webServerAct = false;
  simular.all_simFlags = false;
}


bool stopAllRiego()
{
  led(Boton[bID_bIndex(*multi.id)].led,OFF);
  tic_parpadeoLedZona.detach();
  for(unsigned int i=0;i<NUMZONAS;i++) {
    led(Boton[bID_bIndex(ZONAS[i])].led,OFF);
    if(!stopRiego(ZONAS[i])) return false; 
  }
  return true;
}


void bip(int veces)
{
  for (int i=0; i<veces;i++) {
    led(BUZZER,ON);
    delay(50);
    led(BUZZER,OFF);
    delay(50);
  }
}


void longbip(int veces)
{
  for (int i=0; i<veces;i++) {
    led(BUZZER,ON);
    delay(750);
    led(BUZZER,OFF);
    delay(100);
  }
}


void bipOK(int veces)
{
    led(BUZZER,ON);
    delay(500);
    led(BUZZER,OFF);
    delay(100);
    bip(veces);
}


void bipEND(int veces)
{
    led(BUZZER,ON);
    delay(500);
    led(BUZZER,OFF);
    delay(100);
    bip(veces);
    delay(100);
    led(BUZZER,ON);
    delay(500);
    led(BUZZER,OFF);
}


void blinkPause()
{
  if (!displayOff) {
    if (millis() > lastBlinkPause + DEFAULTBLINKMILLIS) {
      display->clearDisplay();
      displayOff = true;
      lastBlinkPause = millis();
    }
  }
  else {
    if (millis() > lastBlinkPause + DEFAULTBLINKMILLIS) {
      refreshDisplay();
      displayOff = false;
      lastBlinkPause = millis();
    }
  }
}

void blinkPauseError()
{
  if (!displayOff) {
    if (millis() > lastBlinkPause + DEFAULTBLINKMILLIS) {
      display->print(errorText);
      displayOff = true;
      lastBlinkPause = millis();
    }
  }
  else {
    if (millis() > lastBlinkPause + DEFAULTBLINKMILLIS) {
      refreshTime();
      displayOff = false;
      lastBlinkPause = millis();
    }
  }
}


void StaticTimeUpdate(void)
{
  if (Estado.estado == ERROR) return; 
  if (minutes < MINMINUTES) minutes = MINMINUTES;
  if (minutes > MAXMINUTES) minutes = MAXMINUTES;
  display->printTime(minutes, seconds);
}


void refreshDisplay()
{
  display->refreshDisplay();
}


void refreshTime()
{
  display->printTime(T.ShowMinutes(),T.ShowSeconds());
}


void infoDisplay(const char *textDisplay, int dnum, int btype, int bnum) {
  display->print(textDisplay);
  if (btype == LONGBIP) longbip(bnum);
  if (btype == BIP) bip(bnum);
  if (btype == BIPOK) bipOK(bnum);
  display->blink(dnum);
}

String httpGetDomoticz(String message) 
{
  #ifdef TRACE
    Serial.println(F("TRACE: in httpGetDomoticz"));
  #endif
  String tmpStr = "http://" + String(config.domoticz_ip) + ":" + config.domoticz_port + String(message);
  #ifdef DEBUG
    Serial.print(F("TMPSTR: "));Serial.println(tmpStr);
  #endif
  httpclient.begin(client, tmpStr);
  String response = "{}";
  int httpCode = httpclient.GET();
  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) {
      response = httpclient.getString();
      #ifdef EXTRADEBUG1
        Serial.print(F("httpGetDomoticz RESPONSE: "));Serial.println(response);
      #endif
    }
  }
  else {
    if(Estado.estado != ERROR) {
      Serial.printf("[ERROR] httpGetDomoticz: ERROR comunicando con Domoticz error: %s\n", httpclient.errorToString(httpCode).c_str()); 
    }
    return "Err2";
  }
  int pos = response.indexOf("\"status\" : \"ERR");
  if(pos != -1) {
    Serial.println(F("[ERROR] httpGetDomoticz: SE HA DEVUELTO ERROR")); 
    return "ErrX";
  }
  httpclient.end();
  return response;
}


int getFactor(uint16_t idx)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in getFactor"));
  #endif
  if(idx == 0) return 0;
  factorRiegosOK = false;
  if(!checkWifi()) {
    if(NONETWORK) return 999; 
    else {
      statusError(E1,3);
      return 100;
    }
  }
  char JSONMSG[200]="/json.htm?type=devices&rid=%d";
  char message[250];
  sprintf(message,JSONMSG,idx);
  String response = httpGetDomoticz(message);
  if (response.startsWith("Err")) {
    if (NONETWORK) { 
      setEstado(STANDBY);
      return 999;
    }
    if(response == "ErrX") statusError(E3,3);
    else statusError(E2,3);
    #ifdef DEBUG
      Serial.printf("GETFACTOR IDX: %d [HTTP] GET... failed\n", idx);
    #endif
    return 100;
  }

  char* response_pointer = &response[0];
  DynamicJsonDocument jsondoc(2048);
  DeserializationError error = deserializeJson(jsondoc, response_pointer);
  if (error) {
    Serial.print(F("[ERROR] getFactor: deserializeJson() failed: "));
    Serial.println(error.f_str());
    if(!VERIFY) return 100;
    else {
      statusError(E2,3); 
      return 100;
    }
  }
  const char *factorstr = jsondoc["result"][0]["Description"];
  if(factorstr == NULL) {
    #ifdef VERBOSE
      Serial.printf("El idx %d no se ha podido leer del JSON\n",idx);
    #endif
    if(!VERIFY) return 100;
    else {
      statusError(E3,3);
      return 100;
    }
  }
  #ifdef xNAME
    strlcpy(descDomoticz, jsondoc["result"][0]["Name"] | "", sizeof(descDomoticz));
  #endif  
  factorRiegosOK = true;
  long int factor = strtol(factorstr,NULL,10);
  if (factor == 0) {
    if (strlen(factorstr) == 0) return 100;   
    if (!isdigit(factorstr[0])) return 100;   
  }
  return (int)factor;
}

bool queryStatus(uint16_t idx, char *status)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in queryStatus"));
  #endif
  if(!checkWifi()) {
    if(NONETWORK) return true; 
    else {
      Estado.fase=E1;
      return false;
    }
  }
  char JSONMSG[200]="/json.htm?type=devices&rid=%d";
  char message[250];
  sprintf(message,JSONMSG,idx);
  String response = httpGetDomoticz(message);
  if (response.startsWith("Err")) {
    if (NONETWORK) return true;  
    if(response == "ErrX") Estado.fase=E3;
    else Estado.fase=E2;
    Serial.printf("[ERROR] queryStatus IDX: %d [HTTP] GET... failed\n", idx);
    return false;
  }
  char* response_pointer = &response[0];
  DynamicJsonDocument jsondoc(2048);
  DeserializationError error = deserializeJson(jsondoc, response_pointer);
  if (error) {
    Serial.print(F("[ERROR] queryStatus: deserializeJson() failed: "));
    Serial.println(error.f_str());
    Estado.fase=E2;  
    return false;
  }
  const char *actual_status = jsondoc["result"][0]["Status"];
  if(actual_status == NULL) {
    Serial.print(F("[ERROR] queryStatus: deserializeJson() failed: Status not found"));
    Estado.fase=E2;  
    return false;
  }
  #ifdef EXTRADEBUG
    Serial.printf( "queryStatus verificando, status=%s / actual=%s \n" , status, actual_status);
    Serial.printf( "                status_size=%d / actual_size=%d \n" , strlen(status), strlen(actual_status));
  #endif
  if(simular.ErrorVerifyON) {  
    if(strcmp(status, "On")==0) return false; else return true; 
  } 
  if(simular.ErrorVerifyOFF) {  
    if(strcmp(status, "Off")==0) return false; else return true; 
  } 
  if(strcmp(actual_status,status) == 0) return true;
  else{
    if(NONETWORK) return true; 
    #ifdef DEBUG
      Serial.print(F("queryStatus devuelve FALSE, status / actual = "));Serial.print(status);Serial.println(actual_status);
    #endif
    return false;
  }  
}

bool domoticzSwitch(int idx, char *msg, int retries)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in domoticzSwitch"));
  #endif
  if(idx == 0) return true; 
  if(!checkWifi() && !NONETWORK) {
    statusError(E1,3);
    return false;
  }
  char JSONMSG[200]="/json.htm?type=command&param=switchlight&idx=%d&switchcmd=%s";
  char message[250];
  sprintf(message,JSONMSG,idx,msg);
  String response;
  for(int i=0; i<retries; i++) {
     if ((simular.ErrorON && strcmp(msg,"On")==0) || (simular.ErrorOFF && strcmp(msg,"Off")==0)) response = "ErrX"; 
     else if(!NONETWORK) response = httpGetDomoticz(message); 
     if(response == "ErrX") { 
       bip(1);
       Serial.printf("DOMOTICZSWITH IDX: %d fallo en %s (intento %d de %d)\n", idx, msg, i+1, retries);
       delay(DELAYRETRY);
     }
     else break;
  }   
  if (response.startsWith("Err")) {
    if (!errorOFF) {
      if(response == "ErrX") {
        if(strcmp(msg,"On") == 0 ) statusError(E4,3); 
        else statusError(E5,5); 
      }
      else statusError(E2,3); 
    }
    Serial.printf("DOMOTICZSWITH IDX: %d fallo en %s\n", idx, msg);
    return false;
  }
  return true;
}


void flagVerificaciones() 
{
  flagV = ON; 
}

void Verificaciones() 
{   
  #ifdef DEBUG
    leeSerial(); 
  #endif
  #ifdef WEBSERVER
  if (webServerAct) {
    procesaWebServer();
    return;
  }
  #endif
  if (!flagV) return;    
  if (Estado.estado == STANDBY) Serial.print(F("."));
  if (errorOFF) bip(2); 
  if (!NONETWORK && (Estado.estado == STANDBY || (Estado.estado == ERROR && !connected))) {
    if (checkWifi() && Estado.estado!=STANDBY) setEstado(STANDBY); 
    if (connected && falloAP) {
      Serial.println(F("Wifi conectada despues Setup, leemos factor riegos"));
      falloAP = false;
      initFactorRiegos();
    }
    if (!timeOK && connected) initClock();    
  }
  flagV = OFF;
}

void statusError(uint8_t errorID, int n) 
{
  strcpy(errorText, "Err");    
  Estado.estado = ERROR;
  Estado.fase = errorID;
  if (errorID == E0) strcpy(errorText, "Err0");
  else sprintf(errorText, "Err%d", errorID);
  Serial.printf("[statusError]: %s \n", errorText);
  display->print(errorText);
  longbip(n);
}


void parpadeoLedON()
{
  byte estado = ledStatusId(LEDR);
  led(LEDR,!estado);
}
void parpadeoLedZona()
{
  byte estado = ledStatusId(ledID);
  led(ledID,!estado);
}

void parpadeoLedConf()
{
  byte estado = ledStatusId(LEDR);
  led(LEDR,!estado);
  estado = ledStatusId(LEDG);
  led(LEDG,!estado);
}

void ledConf(int estado)
{
  if(estado == ON) 
  {
    led(LEDB,OFF);
    led(LEDG,OFF);
    tic_parpadeoLedConf.attach(0.7,parpadeoLedConf);
  }
  else 
  {
    tic_parpadeoLedConf.detach();   
    led(LEDR,ON);                  
    NONETWORK ? led(LEDB,ON) : led(LEDB,OFF);
    checkWifi();
  }
}

void setupParm()
{
  #ifdef TRACE
    Serial.println(F("TRACE: in setupParm"));
  #endif
  if(clean_FS) cleanFS();
  #ifdef DEBUG
    filesInfo();
    Serial.printf( "initParm= %d \n", initFlags.initParm );
  #endif
  if( initFlags.initParm) {
    Serial.println(F(">>>>>>>>>>>>>>  cargando parametros por defecto  <<<<<<<<<<<<<<"));
    bool bRC = copyConfigFile(defaultFile, parmFile); 
    if(bRC) {
      Serial.println(F("carga parametros por defecto OK"));
      infoDisplay("dEF-", DEFAULTBLINK, BIPOK, 3);
    }  
    else    Serial.println(F("[ERROR] carga parametros por defecto"));
  }
  if (!setupConfig(parmFile, config)) {
    Serial.printf("[ERROR] Leyendo fichero parametros %s \n", parmFile);
    if (!setupConfig(defaultFile, config)) {
      Serial.printf("[ERROR] Leyendo fichero parametros %s \n", defaultFile);
    }
  }
  if (!config.initialized) zeroConfig(config); 
  #ifdef VERBOSE
    if (config.initialized) Serial.print(F("Parametros cargados, "));
    else Serial.print(F("Parametros zero-config, "));
    printParms(config);
  #endif
}


bool setupConfig(const char *p_filename, Config_parm &cfg) 
{
  Serial.printf("Leyendo fichero parametros %s \n", p_filename);
  bool loaded = loadConfigFile(p_filename, config);
  minutes = config.minutes;
  seconds = config.seconds;
  value = ((seconds==0)?minutes:seconds);
  if (loaded) {
    for(int i=0;i<NUMZONAS;i++) {
      int bIndex = bID_bIndex(ZONAS[i]);
      if (bIndex != i) Serial.println(F("\t\t\t@@@@@@@@@@@@  bIndex != zIndex  @@@@@@@@@@"));
      Boton[bIndex].idx = config.botonConfig[i].idx;
      strlcpy(Boton[bIndex].desc, config.botonConfig[i].desc, sizeof(Boton[bIndex].desc));
    }
    #ifdef DEBUG
      printFile(p_filename);
    #endif
    return true;
  }  
  Serial.println(F("[ERROR] parámetros de configuración no cargados"));
  return false;
}


#ifdef DEBUG
  void printMulti()
  {
      Serial.println(F("TRACE: in printMulti"));
      Serial.printf("MULTI Boton_id x%x: size=%d (%s)\n", *multi.id, *multi.size, multi.desc);
      for(int j = 0; j < *multi.size; j++) {
        Serial.printf("  Zona  id: x%x \n", multi.serie[j]);
      }
    Serial.println();
  }

  void leeSerial() 
  {
    if (Serial.available() > 0) {
      String inputSerial = Serial.readString();
      int inputNumber = inputSerial.toInt();
      if ((!inputNumber || inputNumber>6) && inputNumber != 9) {
          Serial.println(F("Teclee: "));
          Serial.println(F("   1 - simular error NTP"));
          Serial.println(F("   2 - simular error apagar riego"));
          Serial.println(F("   3 - simular error encender riego"));
          Serial.println(F("   4 - simular EV no esta ON en Domoticz"));
          Serial.println(F("   5 - simular EV no esta OFF en Domoticz"));
          Serial.println(F("   6 - simular error al salir del PAUSE"));
          Serial.println(F("   9 - anular simulacion errores"));
      }
      switch (inputNumber) {
            case 1:
                Serial.println(F("recibido:   1 - simular error NTP"));
                timeOK = false;
                break;
            case 2:
                Serial.println(F("recibido:   2 - simular error apagar riego"));
                simular.ErrorOFF = true;
                break;
            case 3:
                Serial.println(F("recibido:   3 - simular error encender riego"));
                simular.ErrorON = true;
                break;
            case 4:
                Serial.println(F("recibido:   4 - simular EV no esta ON en Domoticz"));
                simular.ErrorVerifyON = true;
                break;
            case 5:
                Serial.println(F("recibido:   5 - simular EV no esta OFF en Domoticz"));
                simular.ErrorVerifyOFF = true;
                break;
            case 6:
                Serial.println(F("recibido:   6 - simular error al salir del PAUSE"));
                simular.ErrorPause = true;
                break;
            case 9:
                Serial.println(F("recibido:   9 - anular simulacion errores"));
                timeOK = true;                         
                simular.all_simFlags = false;
      }
    }
  }

#endif  
