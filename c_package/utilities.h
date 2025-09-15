/*
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

#include <fprint.h>
#include <glib.h>
#include <glib-unix.h>


int save_data_into_member(FpPrint* print, char** base64);

GPtrArray* gallery_data_load_from_json(const char *json_path);

int save_into_json_file(const char* user_id, char* base64, const char* path);

FpPrint *print_data_load_from_json(const gchar *user_id, const gchar *json_path);
int check_json(const char* path);
