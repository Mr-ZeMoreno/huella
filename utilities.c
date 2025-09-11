/*
 * Utilities for example programs
 *
 * Copyright (C) 2019 Marco Trevisan <marco.trevisan@canonical.com>
 * Copyright (C) 2025 Josecarlos Vidal <josecarlosvidal2@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#define FP_COMPONENT "example-utilities"

#include <libfprint-2/fprint.h>
#include <stdio.h>

#include "utilities.h"


FpDevice* discover_device (GPtrArray * devices) {
    FpDevice *dev;
    int i;

    if (!devices->len)
        return NULL;

    if (devices->len == 1) {
        i = 0;
    } else {
        g_print ("Multiple dispositivos encontrados, escoja uno.\n");

        for (i = 0; i < devices->len; ++i) {
            dev = g_ptr_array_index (devices, i);
            g_print ("[%d] %s (%s) - driver %s\n", i, fp_device_get_device_id (dev), fp_device_get_name (dev), fp_device_get_driver (dev));
        }

        g_print ("> ");
        if (!scanf ("%d%*c", &i))
            return NULL;

        if (i < 0 || i >= devices->len)
            return NULL;
    }

    dev = g_ptr_array_index (devices, i);
    g_print ("Ha seleccionado el dispositivo %s (%s) tomado por el driver %s\n",
            fp_device_get_device_id (dev), fp_device_get_name (dev),
            fp_device_get_driver (dev));

    return dev;
}

const char* finger_to_string (FpFinger finger) {
    switch (finger) {
    case FP_FINGER_LEFT_THUMB:
        return "left thumb";

    case FP_FINGER_LEFT_INDEX:
        return "left index";

    case FP_FINGER_LEFT_MIDDLE:
        return "left middle";

    case FP_FINGER_LEFT_RING:
        return "left ring";

    case FP_FINGER_LEFT_LITTLE:
        return "left little";

    case FP_FINGER_RIGHT_THUMB:
        return "right thumb";

    case FP_FINGER_RIGHT_INDEX:
        return "right index";

    case FP_FINGER_RIGHT_MIDDLE:
        return "right middle";

    case FP_FINGER_RIGHT_RING:
        return "right ring";

    case FP_FINGER_RIGHT_LITTLE:
        return "right little";

    case FP_FINGER_UNKNOWN:
    default:
        return "unknown";
    }
}

FpFinger finger_chooser (void) {
    int i = FP_FINGER_UNKNOWN;

    for (i = FP_FINGER_FIRST; i <= FP_FINGER_LAST; ++i)
        g_print ("  [%d] %s\n", (i - FP_FINGER_FIRST), finger_to_string (i));

    g_print ("> ");
    if (!scanf ("%d%*c", &i))
        return FP_FINGER_UNKNOWN;

    i += FP_FINGER_FIRST;

    if (i < FP_FINGER_FIRST || i > FP_FINGER_LAST)
        return FP_FINGER_UNKNOWN;

    return i;
}

int input_user_email(char **user_email) {
    char buffer[256];  // buffer temporal en la pila

    g_print("Por favor ingrese su correo: ");
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        g_warning("Error leyendo el correo.");
        return -1;
    }

    // Eliminar salto de línea si existe
    buffer[strcspn(buffer, "\n")] = '\0';

    // Asignar memoria y copiar el contenido del buffer
    *user_email = g_strdup(buffer);

    return 0;
}

int save_data_into_member (FpPrint* print, char** base64) {
    g_autoptr(GError) error = NULL;
    g_autofree guchar *data = NULL;
    gsize size;

    guint8 serialized = fp_print_serialize (print, &data, &size, &error);

    if (!serialized) {
        g_warning("No se pudo serializar la plantilla");
        return -1;
    }

    *base64 = g_base64_encode(data, size);

    return 0;
}

static void _device_closed_common(FpDevice *dev, GAsyncResult *res, GMainLoop *loop) {
    g_autoptr(GError) error = NULL;

    fp_device_close_finish(dev, res, &error);

    if (error)
        g_warning("Error al cerrar el dispositivo: %s", error->message);

    if (loop)
        g_main_loop_quit(loop);
}

void fingerprint_device_closed(FpDevice *dev, GAsyncResult *res, void *user_data){
    Session *session = user_data;
    _device_closed_common(dev, res, session->loop);
}

void clear_storage_device_closed(FpDevice *dev, GAsyncResult *res, void *user_data){
    ClearStorageData *clear_storage_data = user_data;
    _device_closed_common(dev, res, clear_storage_data->_session.loop);
}

void data_free(Session* session){
    g_main_loop_unref (session->loop);
    g_free (session);
}

void fingerprint_session_data_free(FingerprintSession* session) {
    g_clear_handle_id(&session->_clear_storage.sigint_handler, g_source_remove);
    g_clear_object(&session->_clear_storage.cancellable);
    data_free((Session*) session);
}

void list_data_free(ListData* session){
    g_list_free_full(session->to_delete, g_object_unref);
    data_free((Session*)session);
}

void enroll_data_free(EnrollData* session){
    fingerprint_session_data_free((FingerprintSession*) session);
}

void verify_data_free(VerifyData* session){
    fingerprint_session_data_free((FingerprintSession*) session);
}

void clear_storage_data_free(ClearStorageData *session) {
    fingerprint_session_data_free((FingerprintSession*) session);
}

void identify_data_free (IdentifyData *session) {
    fingerprint_session_data_free((FingerprintSession*) session);
}

void delete_data_free(DeleteData* session){
    g_object_unref(session->print);
    g_free(session);
}

gboolean sigint_cb (void* session) {
    FingerprintSession* s = session;
    g_cancellable_cancel (s->_clear_storage.cancellable);

    return G_SOURCE_CONTINUE;
}
