/*
 * Example fingerprint enrollment program
 * Enrolls your chosen finger and saves the print to disk
 *
 * Copyright (C) 2007 Daniel Drake <dsd@gentoo.org>
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

#include "glib.h"
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <libfprint-2/fprint.h>
#include <glib-unix.h>

#include "storage.h"
#include "utilities.h"
#include "consts.h"
#include "enroll.h"

static void on_enroll_progress (FpDevice *device, gint completed_stages, FpPrint  *print, gpointer  user_data, GError *error) {
    if (error) {
        g_warning ("Enrolamiento: La etapa %d de %d ha fallado con error: %s", completed_stages, fp_device_get_nr_enroll_stages (device), error->message);
        return;
    }

    g_print ("Enrolamiento: Etapa %d de %d pasada. Yujuu!\n", completed_stages, fp_device_get_nr_enroll_stages (device));
}

static void on_enroll_completed (FpDevice *dev, GAsyncResult *res, void *user_data) {
    EnrollData *enroll_data = user_data;

    g_autoptr(FpPrint) print = NULL;
    g_autoptr(GError) error = NULL;

    print = fp_device_enroll_finish (dev, res, &error);

    if (!error) {
        enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_SUCCESS;

        int b64 = save_data_into_member(print, &enroll_data->base64);

        if (b64 < 0) {
            g_warning("La conversión a base64 ha fallado, código %d", b64);
            enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
        }

       save_into_json_file(enroll_data->user_email, enroll_data->base64, ENROLLED_JSON_PATH);
    } else {
        g_warning ("Enrolamiento fallido, código %s", error->message);
    }

    fp_device_close (dev, NULL, (GAsyncReadyCallback) fingerprint_device_closed, (FingerprintSession*) enroll_data);
}

static void on_device_opened (FpDevice *dev, GAsyncResult *res, void *user_data) {
    EnrollData *enroll_data = user_data;
    FpPrint *print_template;

    g_autoptr(GError) error = NULL;

    if (!enroll_data->user_email){
        g_warning("No se pudo obtener el correo.");
        return;
    }

    g_print("Correo obtenido como %s \n", enroll_data->user_email);

    if (!fp_device_open_finish (dev, res, &error)) {
        g_warning ("Fallo al abrir dispositivo: %s", error->message);
        g_main_loop_quit (enroll_data->_fingerprint._clear_storage._session.loop);
        return;
    }

    g_print("Dispositivo abierto.\n");

    g_print ("Es momento de enrolar tu huella.\n\n");
    g_print ("Necesitarás escanear satisfactoriamente la huella de tu dedo %s %d veces para completar el proceso.\n\n",
            finger_to_string (enroll_data->_fingerprint.finger),
            fp_device_get_nr_enroll_stages (dev));

    g_print ("Escanea tu huella ahora.\n");

    print_template = print_create_template (dev, enroll_data->_fingerprint.finger, enroll_data->update_fingerprint);
    fp_device_enroll (dev, print_template, enroll_data->_fingerprint._clear_storage.cancellable,
                    on_enroll_progress, NULL, NULL, (GAsyncReadyCallback) on_enroll_completed, enroll_data);
}

int enroll(char* email, int finger_number) {
    g_autoptr(FpContext) ctx = NULL;
    g_autoptr(EnrollData) enroll_data = g_new0(EnrollData, 1);
    GPtrArray *all_devices;
    FpDevice *device;
    FpFinger finger;

    // Verificar si el email ya existe
    if (email_exists(email, ENROLLED_JSON_PATH)) {
        g_warning("El correo %s ya tiene una huella registrada.", email);
        return EXIT_FAILURE;
    }

    finger = finger_chooser(finger_number);

    if (finger == FP_FINGER_UNKNOWN) {
        g_warning ("Se seleccionó un dedo desconocido");
        return EXIT_FAILURE;
    }

    // Verificar si el dedo ya fue registrado (por otro email)
    if (finger_exists(finger, ENROLLED_JSON_PATH)) {
        g_warning("La huella del dedo %s ya está registrada.", finger_to_string(finger));
        return EXIT_FAILURE;
    }

    // setenv ("G_MESSAGES_DEBUG", "all", 0);

    ctx = fp_context_new ();

    all_devices = fp_context_get_devices (ctx);
    if (!all_devices) {
        g_warning ("No fue posible obtener los dispositivos");
        return EXIT_FAILURE;
    }

    device = discover_device (all_devices);
    if (!device) {
        g_warning ("No se detectaron dispositivos.");
        return EXIT_FAILURE;
    }

    enroll_data->_fingerprint.finger = finger;
    enroll_data->user_email = email;
    enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
    enroll_data->_fingerprint._clear_storage._session.loop = g_main_loop_new (NULL, FALSE);
    enroll_data->_fingerprint._clear_storage.cancellable = g_cancellable_new ();
    enroll_data->_fingerprint._clear_storage.sigint_handler = g_unix_signal_add_full (G_PRIORITY_HIGH, SIGINT, sigint_cb, enroll_data, NULL);

    fp_device_open (device, enroll_data->_fingerprint._clear_storage.cancellable, (GAsyncReadyCallback) on_device_opened, enroll_data);

    g_main_loop_run (enroll_data->_fingerprint._clear_storage._session.loop);

    return enroll_data->_fingerprint._clear_storage._session.ret_value;
}
