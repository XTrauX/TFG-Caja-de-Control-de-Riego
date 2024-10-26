/**
 * @file Display.h
 * @brief Header file for the Display class which manages a TM1637 7-segment display.
 * 
 * This file contains the definition of the Display class, which provides methods
 * to interact with a TM1637 7-segment display. The class allows for displaying
 * raw data, strings, integers, and time, as well as controlling display behavior
 * such as blinking and clearing.
 * 
 * @note This file is part of the ControlRiego-2.5 project.
 */

#ifndef Display_h
#define Display_h

#include <TM1637.h>
#include <Control.h>

#ifdef NODEMCU
/**
 * @def DISPCLK
 * @brief Define the clock pin for the display when using NODEMCU.
 */
#define DISPCLK D3

/**
 * @def DISPDIO
 * @brief Define the data I/O pin for the display when using NODEMCU.
 */
#define DISPDIO D2
#endif

/**
 * @class Display
 * @brief A class to manage a TM1637 7-segment display.
 * 
 * The Display class provides methods to print various types of data to a TM1637
 * 7-segment display, control display behavior, and refresh the display.
 */
class Display
{
private:
  TM1637 ledDisp; ///< Instance of TM1637 to control the display.
  uint8_t actual[5]; ///< Array to store the current display state.

public:
  /**
   * @brief Construct a new Display object.
   * 
   * @param clk The clock pin for the display.
   * @param dio The data I/O pin for the display.
   */
  Display(uint8_t clk, uint8_t dio);

  /**
   * @brief Print raw data to the display.
   * 
   * @param data Pointer to the raw data to be displayed.
   */
  void printRaw(uint8_t *data);

  /**
   * @brief Print a string to the display.
   * 
   * @param str The string to be displayed.
   */
  void print(const char *str);

  /**
   * @brief Print an integer to the display.
   * 
   * @param num The integer to be displayed.
   */
  void print(int num);

  /**
   * @brief Print time to the display.
   * 
   * @param hours The hours to be displayed.
   * @param minutes The minutes to be displayed.
   */
  void printTime(int hours, int minutes);

  /**
   * @brief Make the display blink.
   * 
   * @param times The number of times the display should blink.
   */
  void blink(int times);

  /**
   * @brief Clear the display.
   */
  void clearDisplay(void);

  /**
   * @brief Check the display state.
   * 
   * @param state The state to check.
   */
  void check(int state);

  /**
   * @brief Refresh the display.
   */
  void refreshDisplay(void);
};

#endif // Display_h