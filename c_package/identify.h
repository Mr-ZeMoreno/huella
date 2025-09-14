#pragma once

#define NO_IDENTIFICADO "<no identificado>"

int fingerprint_init();              // Inicializa contextos, abre dispositivo
const char* fingerprint_identify();  // Corre la identificación
const int fingerprint_enroll(const char* username, int finger_number);
void fingerprint_cleanup();          // Cierra y libera
