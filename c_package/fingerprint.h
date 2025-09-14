/*
 * Biblioteca de enrolamiento e identificación sincrona
 * pensada para ser usada como biblioteca Python.
 *
 * Copyrights (c) 2025 Josecarlos Vidal <jvidalq@usm.cl>
 *
 * Esta biblioteca es software libre; usted puede redistribuirla y/o
 * modificarla bajo los términos de la Licencia Pública Menor de GNU
 * tal como la publica la Free Software Foundation; ya sea
 * la versión 2.1 de la Licencia, o (a su elección) cualquier versión posterior.
 *
 * Esta biblioteca se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTÍA; ni siquiera la garantía implícita de
 * COMERCIALIZACIÓN o IDONEIDAD PARA UN PROPÓSITO PARTICULAR. Consulte la
 * Licencia Pública Menor de GNU para más detalles.
 *
 * Usted debe haber recibido una copia de la Licencia Pública Menor de GNU
 * junto con esta biblioteca; si no es así, escriba a la Free Software Foundation, Inc.,
 * 51 Franklin Street, Quinto Piso, Boston, MA 02110-1301, USA.
 */

#pragma once

int fingerprint_init();              // Inicializa contextos, abre dispositivo
const char* fingerprint_identify();  // Corre la identificación
const int fingerprint_enroll(const char* username, int finger_number); // Corre el enrolamiento
void fingerprint_cleanup();          // Cierra y libera
