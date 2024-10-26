
/**
 * @file Control.h
 * @brief Header file for the irrigation control system.
 *
 * This file contains the definitions, structures, and function prototypes 
 * used in the irrigation control system. It includes necessary libraries 
 * and handles configuration settings for different environments such as 
 * development, release, and demo.
 *
 * The file defines various macros, enums, and structures to manage the 
 * irrigation zones, groups, and buttons. It also includes global variables 
 * and function prototypes for handling the system's operations.
 *
 * @version 2.5
 * @date 2023-10
 * 
 * @note This file is part of the ControlRiego-2.5 project.
 * 
 * @section Libraries
 * - DNSServer.h
 * - WifiUdp.h
 * - WiFiManager.h
 * - SPI.h
 * - NTPClient.h
 * - Time.h
 * - Timezone.h
 * - ClickEncoder.h
 * - CountUpDownTimer.h
 * - ArduinoJson.h
 * - Ticker.h
 * - LittleFS.h
 * - ESP8266HTTPClient.h (if NODEMCU is defined)
 * - ESP8266WiFi.h (if NODEMCU is defined)
 * - ESP8266WebServer.h (if NODEMCU is defined)
 * - ESP8266mDNS.h (if WEBSERVER is defined)
 * - ESP8266HTTPUpdateServer.h (if WEBSERVER is defined)
 * - Display.h
 * - Configure.h
 *
 * @section Macros
 * - VERSION: Defines the version of the system.
 * - Various macros for configuration settings and hardware dependencies.
 *
 * @section Enums
 * - _estados: Defines the different states of the system.
 * - _fases: Defines the different phases of the system.
 * - _flags: Defines various flags used in the system.
 * - _botones: Defines the different buttons used in the system.
 *
 * @section Structures
 * - Grupo_parm: Structure to save a group configuration.
 * - Boton_parm: Structure to save button parameters.
 * - Config_parm: Structure for configurable parameters.
 * - S_MULTI: Structure for a multi-irrigation group.
 * - S_bFLAGS: Union for button flags.
 * - S_initFlags: Structure for initialization flags.
 * - S_simFlags: Union for simulation flags.
 * - S_BOTON: Structure for button details.
 * - S_Estado: Structure for system state.
 *
 * @section Globals
 * - Various global variables and constants used across the modules.
 *
 * @section Functions
 * - Prototypes for various functions used in the system.
 */
#ifndef control_h
  #define control_h
  
  #ifndef _GNU_SOURCE  
   #define _GNU_SOURCE 
  #endif               
  
  #include <DNSServer.h>
  #include <WifiUdp.h>
  #include <WiFiManager.h> 
  #include <SPI.h>
  #include <NTPClient.h>
  #include <Time.h>
  #include <Timezone.h>
  #include <ClickEncoder.h>
  #include <CountUpDownTimer.h>
  #include <ArduinoJson.h>
  #include <Ticker.h>
  #include <LittleFS.h>

  #ifdef NODEMCU
    #include <ESP8266HTTPClient.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h >
    #ifdef WEBSERVER
      #include <ESP8266mDNS.h>
      #include <ESP8266HTTPUpdateServer.h>
    #endif
  #endif

  #include "Display.h"
  #include "Configure.h"

  #ifdef DEVELOP
    //Comportamiento general para PRUEBAS . DESCOMENTAR LO QUE CORRESPONDA
    #define DEBUG
    //#define EXTRADEBUG
    //#define EXTRADEBUG1
    #define TRACE
    //#define EXTRATRACE
    #define VERBOSE
  #endif

  #ifdef RELEASE
    //Comportamiento general para uso normal . DESCOMENTAR LO QUE CORRESPONDA
    //#define DEBUG
    //#define EXTRADEBUG
    //#define TRACE
    //#define EXTRATRACE
    #define VERBOSE
  #endif

  #ifdef DEMO
    //Comportamiento general para DEMO . DESCOMENTAR LO QUE CORRESPONDA
    #define DEBUG
    //#define EXTRADEBUG
    //#define TRACE
    //#define EXTRATRACE
    #define VERBOSE
  #endif

  #ifdef DEVELOP
    #define HOSTNAME "ardomot"
  #else
    #define HOSTNAME "ardomo"
  #endif  

  //-------------------------------------------------------------------------------------
                            #define VERSION  "2.5"
  //-------------------------------------------------------------------------------------

  #define xNAME true 

 
  #define STANDBYSECS         15
  #ifdef RELEASE
    #define DEFAULTMINUTES      10
    #define DEFAULTSECONDS      0
  #endif
  #ifdef DEVELOP
    #define DEFAULTMINUTES      0
    #define DEFAULTSECONDS      10
  #endif
  #ifdef DEMO
    #define DEFAULTMINUTES      0
    #define DEFAULTSECONDS      7
  #endif
  #define DEFAULTBLINK        5
  #define DEFAULTBLINKMILLIS  500
  #define MINMINUTES          0
  #define MAXMINUTES          59  
  #define MINSECONDS          5
  #define HOLDTIME            3000
  #define MAXCONNECTRETRY     10
  #define VERIFY_INTERVAL     15
  #define DEFAULT_SWITCH_RETRIES 5
  #define DELAYRETRY          2000

 //----------------  dependientes del HW   ----------------------------------------

  #ifdef NODEMCU
    #define ENCCLK                D0
    #define ENCDT                 D1
    #define ENCSW                 100
    #define BUZZER                2
    #define HC595_DATA            D8
    #define HC595_LATCH           D4
    #define HC595_CLOCK           D5
    #define CD4021B_CLOCK         D5
    #define CD4021B_LATCH         D6
    #define CD4021B_DATA          D7
    #define LEDR                  4
    #define LEDG                  5
    #define LEDB                  3 
    #define lGRUPO1               6
    #define lGRUPO2               7
    #define lGRUPO3               8
    #define lZONA1                10
    #define lZONA2                11
    #define lZONA3                12
    #define lZONA4                13
    #define lZONA5                14
    #define lZONA6                15
    #define lZONA7                16
  #endif
 //----------------  fin dependientes del HW   ----------------------------------------


  //Para legibilidad del codigo
  #define ON  1
  #define OFF 0
  #define SHOW 1
  #define HIDE 0
  #define READ 1
  #define CLEAR 0
  #define LONGBIP 1
  #define BIP 2
  #define BIPOK 3
  #define BIPEND 4
  #define NOBLINK 0

  //Enumerados para los estados
  enum _estados {
    STANDBY       ,
    REGANDO       ,
    CONFIGURANDO  ,
    TERMINANDO    ,
    PAUSE         ,
    STOP          ,
    ERROR         ,
  };
  #define _ESTADOS "STANDBY" , "REGANDO" , "CONFIGURANDO" , "TERMINANDO" , "PAUSE" , "STOP" , "ERROR"
  //Enumerados para las fases
  enum _fases {
    CERO          = 0,
    E0            = 0xFF,
    E1            = 1,
    E2            = 2,
    E3            = 3,
    E4            = 4,
    E5            = 5,
  };
  //Enumerados para los flags
  enum _flags {
    ENABLED      = 0x01,
    DISABLED     = 0x02,
    ONLYSTATUS   = 0x04,
    ACTION       = 0x08,
    DUAL         = 0x10,
    HOLD         = 0x20,
  };

  //----------------  dependientes del HW   ----------------------------------------
  enum _botones {
    bZONA1      = 0x0001,
    bZONA2      = 0x0002,
    bZONA3      = 0x0004,
    bZONA4      = 0x0008,
    bZONA6      = 0x0010,
    bMULTIRIEGO = 0x0020,
    bZONA7      = 0x0040,
    bZONA5      = 0x0080,
    bSPARE13    = 0x0100,
    bGRUPO3     = 0x0200,
    bGRUPO1     = 0x0400,
    bSTOP       = 0x0800,
    bENCODER    = 0x1000,
    bSPARE15    = 0x2000,
    bSPARE16    = 0x4000,
    bPAUSE      = 0x8000,
  };

  //Pseudobotones
  #define bGRUPO2   0xFF01
  #define bCONFIG   0xFF02

  //----------------  dependientes del HW (número, orden)  ----------------------------
    // lista de todos los botones de zonas de riego disponibles:
  #define _ZONAS  bZONA1 , bZONA2 , bZONA3 , bZONA4 , bZONA5 , bZONA6 , bZONA7
    // lista de todos los botones (selector) de grupos disponibles:
  #define _GRUPOS bGRUPO1 , bGRUPO2 , bGRUPO3
  #define _NUMZONAS            7  // numero de zonas (botones riego individual)
  #define _NUMGRUPOS           3  // numero de grupos multirriego
 //----------------  fin dependientes del HW   ----------------------------------------

  //estructura para salvar un grupo
  struct Grupo_parm {
    uint16_t id;
    int size;
    uint16_t serie[16];  
    char desc[20];
  } ;

  //estructura para salvar parametros de un boton
  struct Boton_parm {
    char  desc[20];
    uint16_t   idx;
  } ;

  //estructura para parametros configurables
  struct Config_parm {
    uint8_t   initialized=0;
    static const int  n_Zonas = _NUMZONAS; 
    Boton_parm botonConfig[n_Zonas];
    uint8_t   minutes = DEFAULTMINUTES; 
    uint8_t   seconds = DEFAULTSECONDS;
    char domoticz_ip[40];
    char domoticz_port[6];
    char ntpServer[40];
    static const int  n_Grupos = _NUMGRUPOS; 
    Grupo_parm groupConfig[n_Grupos];
  };

  struct S_MULTI {
    uint16_t *id;        //apuntador al id del selector grupo en estructura config (bGrupo_x)
    uint16_t serie[16];  //contiene los id de los botones del grupo (bZona_x)
    uint16_t *zserie;    //apuntador a config con las zonas del grupo (Zona_x)
    int *size;           //apuntador a config con el tamaño del grupo
    int w_size;          //variable auxiliar durante ConF
    int actual;          //variable auxiliar durante un multirriego 
    char *desc;          //apuntador a config con la descripcion del grupo
  } ;

  union S_bFLAGS
  {
    uint8_t all_flags;
    struct
    {
      uint8_t enabled       : 1,
              disabled      : 1,
              onlystatus    : 1,
              action        : 1,
              dual          : 1,
              hold          : 1,
              holddisabled  : 1,
              spare0        : 1;
    };
  };

  struct S_initFlags     {
    uint8_t initParm    : 1,
            initWifi      : 1,
            spare1        : 1;
  };

  union S_simFlags
  {
    uint8_t all_simFlags;
    struct
    {
    uint8_t ErrorOFF       : 1,
            ErrorON        : 1,
            ErrorVerifyON  : 1,
            ErrorVerifyOFF : 1,
            ErrorPause     : 1;
    };
  };

  struct S_BOTON {
    uint16_t   id;
    int   estado;
    int   ultimo_estado;
    int   led;
    S_bFLAGS  flags;
    char  desc[20];
    uint16_t   idx;
  } ;

  struct S_Estado {
    uint8_t estado; 
    uint8_t fase; 
  } ;

  const uint16_t ZONAS[] = {_ZONAS};
  const uint16_t GRUPOS[]  = {_GRUPOS};
  const int NUMZONAS = sizeof(ZONAS)/sizeof(ZONAS[0]); // (7) numero de zonas (botones riego individual)
  const int NUMGRUPOS = sizeof(GRUPOS)/sizeof(GRUPOS[0]); // (3) numero de grupos multirriego
  const char nEstado[][15] = {_ESTADOS};

   //Globales a todos los módulos
  #ifdef __MAIN__

    S_BOTON Boton [] =  { 
      //ID,          S   uS  LED          FLAGS                             DESC          IDX
      {bZONA1   ,   0,  0,  lZONA1   ,   ENABLED | ACTION,                 "ZONA1",       0},
      {bZONA2 ,     0,  0,  lZONA2 ,     ENABLED | ACTION,                 "ZONA2",       0},
      {bZONA3    ,  0,  0,  lZONA3    ,  ENABLED | ACTION,                 "ZONA3",       0},
      {bZONA4    ,  0,  0,  lZONA4    ,  ENABLED | ACTION,                 "ZONA4",       0},
      {bZONA5    ,  0,  0,  lZONA5    ,  ENABLED | ACTION,                 "ZONA5",       0},
      {bZONA6 ,     0,  0,  lZONA6 ,     ENABLED | ACTION,                 "ZONA6",       0},
      {bZONA7  ,    0,  0,  lZONA7  ,    ENABLED | ACTION,                 "ZONA7",       0},
      {bSPARE13,    0,  0,  0,           DISABLED,                         "spare13",     0},
      {bSPARE15,    0,  0,  0,           DISABLED,                         "spare15",     0},
      {bSPARE16,    0,  0,  0,           DISABLED,                         "spare16",     0},
      {bENCODER,    0,  0,  0,           ENABLED | ONLYSTATUS | DUAL,      "ENCODER",     0},
      {bMULTIRIEGO, 0,  0,  0,           ENABLED | ACTION,                 "MULTIRIEGO",  0},
      {bGRUPO1,     0,  0,  lGRUPO1,     ENABLED | ONLYSTATUS | DUAL,      "GRUPO1",      0},
      {bGRUPO2  ,   0,  0,  lGRUPO2  ,   DISABLED,                         "GRUPO2",      0},
      {bGRUPO3,     0,  0,  lGRUPO3,     ENABLED | ONLYSTATUS | DUAL,      "GRUPO3",      0},
      {bPAUSE,      0,  0,  0,           ENABLED | ACTION | DUAL | HOLD,   "PAUSE",       0},
      {bSTOP,       0,  0,  0,           ENABLED | ACTION | DUAL,          "STOP",        0},
      {bCONFIG,     0,  0,  0,           DISABLED,                         "CONFIG",      0}
    };
    int NUM_S_BOTON = sizeof(Boton)/sizeof(Boton[0]);

    S_MULTI multi;  
    S_initFlags initFlags ;
    bool connected;
    bool NONETWORK;
    bool falloAP;
    bool saveConfig = false;
    
    const char *parmFile = "/config_parm.json";       
    const char *defaultFile = "/config_default.json"; 

  #else
    extern S_BOTON Boton [];
    extern S_MULTI multi;
    extern S_initFlags initFlags;
    extern bool connected;
    extern bool NONETWORK;
    extern bool falloAP;
    extern bool saveConfig;
    extern int NUM_S_BOTON;
    extern const char *parmFile;       
    extern const char *defaultFile; 


  #endif

  #ifdef __MAIN__
    //Globales a este módulo
    #ifdef NODEMCU
      //Segun la arquitectura
      WiFiClient client;
      HTTPClient httpclient;
      WiFiUDP    ntpUDP;
    #endif
    CountUpDownTimer T(DOWN);
    S_Estado Estado;
    S_BOTON  *boton;
    S_BOTON  *ultimoBoton;
    S_simFlags simular; // estructura flags para simular errores
    Config_parm config; //estructura parametros configurables y runtime
    ClickEncoder *Encoder;
    Display      *display;
    Configure    *configure;
    NTPClient timeClient(ntpUDP,config.ntpServer);
    Ticker tic_parpadeoLedON;    //para parpadeo led ON (LEDR)
    Ticker tic_parpadeoLedZona;  //para parpadeo led zona de riego
    Ticker tic_parpadeoLedConf;  //para parpadeo led(s) indicadores de modo configuracion
    Ticker tic_verificaciones;   //para verificaciones periodicas
    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
    TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
    Timezone CE(CEST, CET);
    TimeChangeRule *tcr;
    time_t utc;
    time_t lastRiegos[NUMZONAS];
    uint factorRiegos[NUMZONAS];
    uint8_t minutes;
    uint8_t seconds;
    char  descDomoticz[20];
    int value;
    int savedValue;
    int ledID = 0;
    bool tiempoTerminado;
    bool reposo = false;
    unsigned long standbyTime;
    bool displayOff = false;
    unsigned long lastBlinkPause;
    bool multirriego = false;
    bool multiSemaforo = false;
    bool holdPause = false;
    unsigned long countHoldPause;
    bool flagV = OFF;
    int ledState = LOW;
    bool timeOK = false;
    bool factorRiegosOK = false;
    bool errorOFF = false;
    bool webServerAct = false;
    bool VERIFY;
    bool encoderSW = false;
    char errorText[7];
    bool clean_FS = false;



  #endif
  // Function prototypes

  /**
   * @brief Turns off all LEDs.
   */
  void apagaLeds(void);

  /**
   * @brief Emits a beep sound.
   * @param duration Duration of the beep.
   */
  void bip(int duration);

  /**
   * @brief Emits a beep sound indicating OK status.
   * @param duration Duration of the beep.
   */
  void bipOK(int duration);

  /**
   * @brief Emits a beep sound indicating end status.
   * @param duration Duration of the beep.
   */
  void bipEND(int duration);

  /**
   * @brief Gets the index of a button by its ID.
   * @param id Button ID.
   * @return Index of the button.
   */
  int bID_bIndex(uint16_t id);

  /**
   * @brief Gets the index of a zone by its ID.
   * @param id Zone ID.
   * @return Index of the zone.
   */
  int bID_zIndex(uint16_t id);

  /**
   * @brief Handles the blinking of the pause LED.
   */
  void blinkPause(void);

  /**
   * @brief Handles the blinking of the pause LED during an error.
   */
  void blinkPauseError(void);

  /**
   * @brief Performs system checks.
   */
  void check(void);

  /**
   * @brief Checks the WiFi connection status.
   * @return True if WiFi is connected, false otherwise.
   */
  bool checkWifi(void);

  /**
   * @brief Cleans the file system.
   */
  void cleanFS(void);

  /**
   * @brief Copies a configuration file.
   * @param src Source file path.
   * @param dest Destination file path.
   * @return True if the copy was successful, false otherwise.
   */
  bool copyConfigFile(const char* src, const char* dest);

  /**
   * @brief Dims the LEDs.
   */
  void dimmerLeds(void);

  /**
   * @brief Displays the group information.
   * @param group Pointer to the group ID.
   * @param size Size of the group.
   */
  void displayGrupo(uint16_t *group, int size);

  /**
   * @brief Sends a switch command to Domoticz.
   * @param idx Index of the switch.
   * @param desc Description of the switch.
   * @param status Status to set.
   * @return True if the command was successful, false otherwise.
   */
  bool domoticzSwitch(int idx, char *desc, int status);

  /**
   * @brief Turns on all LEDs.
   */
  void enciendeLeds(void);

  /**
   * @brief Ends the web server.
   */
  void endWS(void);

  /**
   * @brief Displays file information.
   */
  void filesInfo(void);

  /**
   * @brief Sets the verification flag.
   */
  void flagVerificaciones(void);

  /**
   * @brief Gets the factor for a given ID.
   * @param id ID to get the factor for.
   * @return Factor for the given ID.
   */
  int getFactor(uint16_t id);

  /**
   * @brief Gets the status of multiple items.
   * @return Status of the items.
   */
  uint16_t getMultiStatus(void);

  /**
   * @brief Sends an HTTP GET request to Domoticz.
   * @param url URL to send the request to.
   * @return Response from Domoticz.
   */
  String *httpGetDomoticz(String *url);

  /**
   * @brief Displays information on the display.
   * @param text Text to display.
   * @param x X position.
   * @param y Y position.
   * @param size Size of the text.
   */
  void infoDisplay(const char *text, int x, int y, int size);

  /**
   * @brief Initializes the CD4021B chip.
   */
  void initCD4021B(void);

  /**
   * @brief Initializes the clock.
   */
  void initClock(void);

  /**
   * @brief Initializes the irrigation factors.
   */
  void initFactorRiegos(void);

  /**
   * @brief Initializes the HC595 chip.
   */
  void initHC595(void);

  /**
   * @brief Initializes the last irrigation times.
   */
  void initLastRiegos(void);

  /**
   * @brief Initializes the LEDs.
   */
  void initLeds(void);

  /**
   * @brief Initializes the irrigation for a given ID.
   * @param id ID to initialize the irrigation for.
   * @return True if the initialization was successful, false otherwise.
   */
  bool initRiego(uint16_t id);

  /**
   * @brief Controls the state of a LED.
   * @param led LED ID.
   * @param state State to set.
   */
  void led(uint8_t led, int state);

  /**
   * @brief Controls the configuration LEDs.
   * @param state State to set.
   */
  void ledConf(int state);

  /**
   * @brief Controls the RGB LED.
   * @param r Red component.
   * @param g Green component.
   * @param b Blue component.
   */
  void ledRGB(int r, int g, int b);

  /**
   * @brief Gets the status of a LED by its ID.
   * @param id LED ID.
   * @return True if the LED is on, false otherwise.
   */
  bool ledStatusId(int id);

  /**
   * @brief Reads data from the serial port.
   */
  void leeSerial(void);

  /**
   * @brief Loads a configuration file.
   * @param filename Path to the configuration file.
   * @param config Configuration structure to load into.
   * @return True if the file was loaded successfully, false otherwise.
   */
  bool loadConfigFile(const char* filename, Config_parm& config);

  /**
   * @brief Loads the default signal.
   * @param signal Signal to load.
   */
  void loadDefaultSignal(uint signal);

  /**
   * @brief Emits a long beep sound.
   * @param duration Duration of the beep.
   */
  void longbip(int duration);

  /**
   * @brief Displays memory information.
   */
  void memoryInfo(void);

  /**
   * @brief Handles the blinking of the ON LED.
   */
  void parpadeoLedON(void);

  /**
   * @brief Handles the blinking of the zone LED.
   */
  void parpadeoLedZona(void);

  /**
   * @brief Parses the inputs.
   * @param state State to parse.
   * @return Pointer to the parsed button.
   */
  S_BOTON *parseInputs(bool state);

  /**
   * @brief Prints a character array.
   * @param array Character array to print.
   * @param size Size of the array.
   */
  void printCharArray(char* array, size_t size);

  /**
   * @brief Prints the contents of a file.
   * @param filename Path to the file.
   */
  void printFile(const char* filename);

  /**
   * @brief Prints the multi-irrigation information.
   */
  void printMulti(void);

  /**
   * @brief Prints the multi-irrigation group information.
   * @param config Configuration structure.
   * @param groupIndex Index of the group.
   */
  void printMultiGroup(Config_parm& config, int groupIndex);

  /**
   * @brief Prints the configuration parameters.
   * @param config Configuration structure.
   */
  void printParms(Config_parm& config);

  /**
   * @brief Processes the buttons.
   */
  void procesaBotones(void);

  /**
   * @brief Processes the multi-irrigation button.
   * @return True if the button was processed successfully, false otherwise.
   */
  bool procesaBotonMultiriego(void);

  /**
   * @brief Processes the pause button.
   */
  void procesaBotonPause(void);

  /**
   * @brief Processes the stop button.
   */
  void procesaBotonStop(void);

  /**
   * @brief Processes the zone button.
   */
  void procesaBotonZona(void);

  /**
   * @brief Processes the encoder.
   */
  void procesaEncoder(void);

  /**
   * @brief Processes the system states.
   */
  void procesaEstados(void);

  /**
   * @brief Processes the configuring state.
   */
  void procesaEstadoConfigurando(void);

  /**
   * @brief Processes the error state.
   */
  void procesaEstadoError(void);

  /**
   * @brief Processes the irrigating state.
   */
  void procesaEstadoRegando(void);

  /**
   * @brief Processes the standby state.
   */
  void procesaEstadoStandby(void);

  /**
   * @brief Processes the terminating state.
   */
  void procesaEstadoTerminando(void);

  /**
   * @brief Processes the stop state.
   */
  void procesaEstadoStop(void);

  /**
   * @brief Processes the pause state.
   */
  void procesaEstadoPause(void);

  /**
   * @brief Processes the web server.
   */
  void procesaWebServer(void);

  /**
   * @brief Queries the status of a given ID.
   * @param id ID to query.
   * @param status Status to query.
   * @return True if the query was successful, false otherwise.
   */
  bool queryStatus(uint16_t id, char *status);

  /**
   * @brief Refreshes the time.
   */
  void refreshTime(void);

  /**
   * @brief Refreshes the display.
   */
  void refreshDisplay(void);

  /**
   * @brief Resets the flags.
   */
  void resetFlags(void);

  /**
   * @brief Resets the LEDs.
   */
  void resetLeds(void);

  /**
   * @brief Saves a configuration file.
   * @param filename Path to the configuration file.
   * @param config Configuration structure to save.
   * @return True if the file was saved successfully, false otherwise.
   */
  bool saveConfigFile(const char* filename, Config_parm& config);

  /**
   * @brief Sets the system state.
   * @param state State to set.
   */
  void setEstado(uint8_t state);

  /**
   * @brief Sets the multi-irrigation by ID.
   * @param id ID to set.
   * @param config Configuration structure.
   * @return Index of the multi-irrigation.
   */
  int setMultibyId(uint16_t id, Config_parm& config);

  /**
   * @brief Sets up the configuration.
   * @param filename Path to the configuration file.
   * @param config Configuration structure.
   * @return True if the setup was successful, false otherwise.
   */
  bool setupConfig(const char* filename, Config_parm& config);

  /**
   * @brief Sets up the system state.
   */
  void setupEstado(void);

  /**
   * @brief Initializes the setup.
   */
  void setupInit(void);

  /**
   * @brief Sets up the parameters.
   */
  void setupParm(void);

  /**
   * @brief Sets up the WiFi manager.
   * @param config Configuration structure.
   */
  void setupRedWM(Config_parm& config);

  /**
   * @brief Sets up the web server.
   */
  void setupWS(void);

  /**
   * @brief Starts the configuration portal.
   * @param config Configuration structure.
   */
  void starConfigPortal(Config_parm& config);

  /**
   * @brief Updates the static time.
   */
  void StaticTimeUpdate(void);

  /**
   * @brief Sets the error status.
   * @param error Error code.
   * @param n Number of errors.
   */
  void statusError(uint8_t error, int n);

  /**
   * @brief Stops the irrigation for a given ID.
   * @param id ID to stop the irrigation for.
   * @return True if the irrigation was stopped successfully, false otherwise.
   */
  bool stopRiego(uint16_t id);

  /**
   * @brief Stops all irrigation.
   * @return True if all irrigation was stopped successfully, false otherwise.
   */
  bool stopAllRiego(void);

  /**
   * @brief Tests a button.
   * @param id Button ID.
   * @param state State to test.
   * @return True if the button test was successful, false otherwise.
   */
  bool testButton(uint16_t id, bool state);

  /**
   * @brief Calculates the time by factor.
   * @param factor Factor to calculate.
   * @param minutes Pointer to store the minutes.
   * @param seconds Pointer to store the seconds.
   */
  void timeByFactor(int factor, uint8_t *minutes, uint8_t *seconds);

  /**
   * @brief Displays the last irrigation times.
   * @param zone Zone to display.
   */
  void ultimosRiegos(int zone);

  /**
   * @brief Performs periodic verifications.
   */
  void Verificaciones(void);

  /**
   * @brief Clears the WiFi signal.
   * @param signal Signal to clear.
   */
  void wifiClearSignal(uint signal);

  /**
   * @brief Resets the configuration.
   * @param config Configuration structure.
   */
  void zeroConfig(Config_parm& config);
#endif
