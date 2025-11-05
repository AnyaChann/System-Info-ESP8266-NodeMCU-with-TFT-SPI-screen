/*
 * Config Portal HTML Generators
 * Separated for better performance and maintainability
 */

#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>

class ConfigPortal {
public:
  // HTML generators
  static String generateServerConfigHTML();
  static String generateSuccessHTML(const String& apSSID, const String& serverIP, uint16_t serverPort);
  static String generateTestingHTML();
  static String generateErrorHTML(const char* error);
};

#endif // CONFIG_PORTAL_H
