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

#include "fp-device.h"

#include <json-glib/json-glib.h>
#include <fprint.h>

#include "utilities.h"
#include "fingerprint.h"

#define ENROLLED_JSON_PATH "enrolled.json"

static FpContext *ctx = NULL;
static FpDevice *device = NULL;
static char user_buffer[256] = "<no identificado>";
static char* base64 = NULL;

int fingerprint_init() {
    ctx = fp_context_new();
    if (!ctx) return -1;

    GPtrArray *devices = fp_context_get_devices(ctx);
    if (!devices || devices->len == 0) return -2;

    device = g_ptr_array_index(devices, 0);
    g_object_ref(device);
    g_ptr_array_unref(devices);

    if (!fp_device_has_feature(device, FP_DEVICE_FEATURE_IDENTIFY)) return -3;

    // Abrimos el dispositivo sin async
    GError *error = NULL;
    if (!fp_device_open_sync(device, NULL, &error)) {
        g_warning("Error abriendo el dispositivo: %s", error->message);
        g_clear_error(&error);
        return -4;
    }

    return 0;
}

const int fingerprint_enroll(const char *username, int finger_number){
    FpPrint* template = fp_print_new(device);
    GError *error = NULL;

    fp_print_set_finger(template, finger_number);
    fp_print_set_username(template, username);

    if (!fp_device_enroll_sync(device, template, NULL, NULL, NULL, &error)){
        if (error) {
            g_warning("Enrolamiento fallido: %s", error->message);
            g_clear_error(&error);
        }

        return -1;
    }

    int r = save_data_into_member(template, &base64);
    g_print("Huella insertada en la estructura\n");

    if (r < 0) {
        g_warning("La conversión a base64 ha fallado, código %d", r);
        return -2;
    }

    gboolean saved_ok = FALSE;

    r = save_into_json_file(username, base64, ENROLLED_JSON_PATH);
    saved_ok = (r == 0);
    g_print("Huella insertada en el JSON\n");

    if (!saved_ok) {
        g_warning("El guardado o actualización en JSON ha fallado.");
        return -3;
    }

    g_print("La huella ha sido registrada con éxito\n");

    return 0;
};

const char* fingerprint_identify() {
    GPtrArray *gallery = gallery_data_load_from_json(ENROLLED_JSON_PATH);
    if (!gallery) return "<error galería>";

    GError *error = NULL;
    FpPrint *match = NULL;
    FpPrint *scanned = NULL;

    if (!fp_device_identify_sync(device, gallery, NULL, NULL, NULL,&match, &scanned, &error)) {
        if (error) {
            g_warning("Identificación fallida: %s", error->message);
            g_clear_error(&error);
        }
        g_ptr_array_unref(gallery);
        return "<fallo identificación>";
    }

    if (match) {
        const char *username = fp_print_get_username(match);
        strncpy(user_buffer, username ? username : "<desconocido>", sizeof(user_buffer));
        user_buffer[sizeof(user_buffer)-1] = '\0';
    } else {
        strncpy(user_buffer, "<no identificado>", sizeof(user_buffer));
    }

    g_ptr_array_unref(gallery);
    return user_buffer;
}

void fingerprint_cleanup() {
    if (device) {
        fp_device_close_sync(device, NULL, NULL);
        g_object_unref(device);
        device = NULL;
    }

    if (ctx) {
        g_object_unref(ctx);
        ctx = NULL;
    }
}

int main(){
    int ret = fingerprint_init();

    if (ret != 0) return ret;

    // const int i = fingerprint_enroll("jose", 6);

    // g_print("%d", i);

    const char* c = fingerprint_identify();
    printf("%s\n", c);

    // fingerprint_cleanup();
}
