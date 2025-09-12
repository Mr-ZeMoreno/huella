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

#include "fp-device.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"

#include <fprint.h>
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
        return "pulgar izquierdo";

    case FP_FINGER_LEFT_INDEX:
        return "indice izquierdo";

    case FP_FINGER_LEFT_MIDDLE:
        return "medio izquierdo";

    case FP_FINGER_LEFT_RING:
        return "anular izquierdo";

    case FP_FINGER_LEFT_LITTLE:
        return "meñique izquierdo";

    case FP_FINGER_RIGHT_THUMB:
        return "pulgar derecho";

    case FP_FINGER_RIGHT_INDEX:
        return "indice derecho";

    case FP_FINGER_RIGHT_MIDDLE:
        return "medio derecho";

    case FP_FINGER_RIGHT_RING:
        return "anular derecho";

    case FP_FINGER_RIGHT_LITTLE:
        return "meñique derecho";

    case FP_FINGER_UNKNOWN:
    default:
        return "desconocido";
    }
}

FpFinger finger_chooser(int value) {
    if (value < FP_FINGER_FIRST || value > FP_FINGER_LAST)
        return FP_FINGER_UNKNOWN;

    return (FpFinger)value;
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

    if (print == NULL) {
        g_warning("La huella es NULL.");
        return -1;
    }

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

    if (!fp_device_close_finish(dev, res, &error))
        g_warning("Error al cerrar el dispositivo: %s", error->message);
    else
        g_print("Dispositivo cerrado correctamente.\n");


    if (loop)
        g_main_loop_quit(loop);
}

void fingerprint_device_open(FpDevice *dev, GAsyncResult *res){
    g_autoptr(GError) error = NULL;

    // Intentar abrir el dispositivo y manejar el error si ocurre
    if (!fp_device_open_finish(dev, res, &error)) {
        g_warning("No se pudo abrir el dispositivo: %s", error->message);
        return;
    }

    g_print("Dispositivo abierto correctamente.\n");
}

void fingerprint_device_closed(FpDevice *dev, GAsyncResult *res, void *user_data){
    Session *session = user_data;
    _device_closed_common(dev, res, session->loop);
}

void fingerprint_quit(FpDevice *dev, FingerprintSession* session){
    if (!fp_device_is_open (dev)) {
        g_main_loop_quit (session->_clear_storage._session.loop);
        return;
    }

    fp_device_close (dev, NULL, (GAsyncReadyCallback) fingerprint_device_closed, session);
}

static void enroll_device_closed(FpDevice* dev, GAsyncResult *res,FingerprintSession* session){

    g_autoptr(GError) error = NULL;

    fp_device_close_finish (dev, res, &error);

    if (error)
      g_warning ("Failed closing device %s", error->message);

    g_main_loop_quit (session->_clear_storage._session.loop);
}

void enroll_quit(FpDevice *dev, FingerprintSession* session){
    if (!fp_device_is_open (dev)) {
        g_main_loop_quit (session->_clear_storage._session.loop);
        return;
    }

    fp_device_close (dev, NULL, (GAsyncReadyCallback) enroll_device_closed, session);
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

void verify_data_free(VerifyData *data) {
    if (!data) return;
    g_free(data->user_email);
    if (data->_fingerprint._clear_storage._session.loop)
        g_main_loop_unref(data->_fingerprint._clear_storage._session.loop);
    if (data->_fingerprint._clear_storage.cancellable)
        g_object_unref(data->_fingerprint._clear_storage.cancellable);
    if (data->_fingerprint._clear_storage.sigint_handler)
        g_source_remove(data->_fingerprint._clear_storage.sigint_handler);
    g_free(data);
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

gboolean sigint_cb(void* user_data) {
    EnrollData *enroll_data = user_data;

    g_print("Cancelando enrolamiento...\n");

    g_cancellable_cancel(enroll_data->_fingerprint._clear_storage.cancellable);

    return G_SOURCE_CONTINUE; // para indicar que manejamos la señal
}
