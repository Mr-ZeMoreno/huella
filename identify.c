/*
 * Example fingerprint verification program, which verifies the
 * finger which has been previously enrolled to disk.
 *
 * Copyright (C) 2020 Marco Trevisan <marco.trevisan@canonical.com>
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

#include "glib.h"

#include <json-glib/json-glib.h>
#include <libfprint-2/fprint.h>
#include <glib-unix.h>

#include "storage.h"
#include "utilities.h"
#include "consts.h"

static void identify_quit (FpDevice* dev, IdentifyData* identify_data) {
    if (!fp_device_is_open (dev)) {
        g_main_loop_quit (identify_data->_fingerprint._clear_storage._session.loop);
        return;
    }

    fp_device_close (dev, NULL, (GAsyncReadyCallback) fingerprint_device_closed, identify_data);
}

static void start_identification (FpDevice* dev, IdentifyData* identify_data);

static void on_identify_completed (FpDevice *dev, GAsyncResult *res, void *user_data) {
    IdentifyData *identify_data = user_data;

    g_autoptr(FpPrint) print = NULL;
    g_autoptr(FpPrint) match = NULL;
    g_autoptr(GError) error = NULL;

    if (!fp_device_identify_finish (dev, res, &match, &print, &error)) {
        g_warning ("Fallo al identificar huella: %s", error->message);
        identify_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;

        if (error->domain != FP_DEVICE_RETRY) {
            identify_quit (dev, identify_data);
            return;
        }
    }

    identify_quit (dev, identify_data);
}

static void on_identify_cb (FpDevice *dev, FpPrint *match, FpPrint *print, gpointer user_data, GError *error) {
    if (error) {
        g_warning ("Reporte de indentificador: Ninguna huella coincidió: %s",  error->message);
        return;
    }

    if (match) {
        const gchar *user_id = fp_print_get_username(match);

        g_print("IDENTIFICADO\n");
        g_print("Usuario: %s\n", user_id ? user_id : "<desconocido>");
    } else {
        g_debug("Reporte de indentificador: Ninguna huella coincidió: %s",  error->message);
        g_print ("NO IDENTIFICADO\n");
    }
}

static void start_identification (FpDevice *dev, IdentifyData *identify_data) {
    g_autoptr(GPtrArray) gallery = gallery_data_load_from_json(ENROLLED_JSON_PATH); // Punto clave 3 - Obtenemos la galería de muestras

    if (!gallery) {
        identify_quit (dev, identify_data);
        return;
    }

    g_print ("Galleria cargada. Hora de identificar!\n");
    fp_device_identify (dev, gallery, identify_data->_fingerprint._clear_storage.cancellable, on_identify_cb,
                        identify_data, NULL, (GAsyncReadyCallback) on_identify_completed, identify_data); // Punto clave 4 - Identificar
}

static void on_device_opened (FpDevice *dev, GAsyncResult *res, void *user_data) {
    IdentifyData *identify_data = user_data;

    g_autoptr(GError) error = NULL;

    if (!fp_device_open_finish (dev, res, &error)) {
        g_warning ("Fallo al abrir el dispositivo: %s", error->message);
        identify_quit (dev, identify_data);
        return;
    }

    g_print ("Dispositivo abierto. ");

    start_identification (dev, identify_data); // Punto clave 2
}

int identify() {
    g_autoptr(FpContext) ctx = NULL;
    g_autoptr(IdentifyData) identify_data = NULL;
    GPtrArray *todos_los_dispositivos;
    FpDevice *dispositivo;

    // setenv ("G_MESSAGES_DEBUG", "all", 0);
    // setenv ("LIBUSB_DEBUG", "3", 0);

    ctx = fp_context_new ();

    todos_los_dispositivos = fp_context_get_devices (ctx);
    if (!todos_los_dispositivos) {
        g_warning ("Impossible to get devices");
        return EXIT_FAILURE;
    }

    dispositivo = discover_device (todos_los_dispositivos);
    if (!dispositivo) {
        g_warning ("No devices detected.");
        return EXIT_FAILURE;
    }

    if (!fp_device_has_feature (dispositivo, FP_DEVICE_FEATURE_IDENTIFY)) {
        g_warning ("Device %s does not support identification.",
                    fp_device_get_name (dispositivo));
        return EXIT_FAILURE;
    }

    identify_data = g_new(IdentifyData, 1);
    identify_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
    identify_data->_fingerprint._clear_storage._session.loop = g_main_loop_new (NULL, FALSE);
    identify_data->_fingerprint._clear_storage.cancellable = g_cancellable_new ();
    identify_data->_fingerprint._clear_storage.sigint_handler = g_unix_signal_add_full (G_PRIORITY_HIGH, SIGINT, sigint_cb, identify_data, NULL);
    fp_device_open (dispositivo, identify_data->_fingerprint._clear_storage.cancellable, (GAsyncReadyCallback) on_device_opened, identify_data); // Punto clave 1

    g_main_loop_run (identify_data->_fingerprint._clear_storage._session.loop);

    return identify_data->_fingerprint._clear_storage._session.ret_value;
}
