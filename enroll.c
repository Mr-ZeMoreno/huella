/*
 * Example fingerprint enrollment program
 * Enrolls your chosen finger and saves the print to disk
 * Copyright (C) 2007 Daniel Drake <dsd@gentoo.org>
 * Copyright (C) 2019 Marco Trevisan <marco.trevisan@canonical.com>
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

#define FP_COMPONENT "example-enroll"

#include <stdio.h>
#include <libfprint-2/fprint.h>
#include <glib-unix.h>

#include "storage.h"
#include "utilities.h"


static void on_enroll_completed (FpDevice *dev, GAsyncResult *res, void *user_data) {
    EnrollData *enroll_data = user_data;

    g_autoptr(FpPrint) print = NULL;
    g_autoptr(GError) error = NULL;

    print = fp_device_enroll_finish (dev, res, &error);

    if (!error)
    {
        enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_SUCCESS;

        if (fp_device_has_feature (dev, FP_DEVICE_FEATURE_STORAGE))
            g_debug ("El dispositivo tiene almacenamiento, guardando una muestra de referencia localmente");
        else
            g_debug ("El dispositivo no tiene almacenamiento, guardando la muestra solo en local");

        /* Even if the device has storage, it may not be able to save all the
        * metadata that the print contains, so we can always save a local copy
        * containing the handle to the device print */
        int r = print_data_save (print, enroll_data->_fingerprint.finger,
                                enroll_data->update_fingerprint);
        if (r < 0) {
            g_warning ("El guardado de datos ha fallado, código %d", r);
            enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
        }
    }
    else {
        g_warning ("Enrolamiento fallido, código %s", error->message);
    }

    fp_device_close (dev, NULL, (GAsyncReadyCallback) fingerprint_device_closed, (FingerprintSession*) enroll_data);
}

static void on_enroll_progress (FpDevice *device,
                                gint      completed_stages,
                                FpPrint  *print,
                                gpointer  user_data,
                                GError   *error) {
    if (error) {
        g_warning ("Enrolamiento: La etapa %d de %d ha fallado con error: %s",
                    completed_stages,
                    fp_device_get_nr_enroll_stages (device),
                    error->message);

        return;
    }

    if (print && fp_print_get_image (print) &&
        print_image_save (print, "enrolled.pgm"))
        printf ("Imagen escaneada escrita en 'enrolled.pgm'\n");

    printf ("Enrolamiento: Etapa %d de %d pasada. Yujuu!\n", completed_stages,
            fp_device_get_nr_enroll_stages (device));
}

static gboolean should_update_fingerprint (void) {
    int update_choice;
    gboolean update_fingerprint = FALSE;

    printf ("¿Deseas actualizar una huella existente en lugar de reemplazarla (si ya existe)?\n"
            "Ingresa S/s para actualizar y N/n para no hacer nada\n");
    update_choice = getchar ();
    if (update_choice == EOF) {
        g_warning ("EOF han sido detectados!, Saliendo...");
        return EXIT_FAILURE;
    }

    switch (update_choice) {
        case 's':
        case 'S':
            update_fingerprint = TRUE;
            break;

        case 'n':
        case 'N':
            update_fingerprint = FALSE;
            break;

        default:
            g_warning ("Elección inválida '%c', debería haber sido S/s o N/n.", update_choice);
            return EXIT_FAILURE;
    }
    return update_fingerprint;
}

static void on_device_opened (FpDevice *dev, GAsyncResult *res, void *user_data) {
    EnrollData *enroll_data = user_data;
    FpPrint *print_template;

    g_autoptr(GError) error = NULL;

    if (!fp_device_open_finish (dev, res, &error)) {
        g_warning ("Fallo al abrir dispositivo: %s", error->message);
        g_main_loop_quit (enroll_data->_fingerprint._clear_storage._session.loop);
        return;
    }

    printf ("Dispositivo abierto.\n");

    if (fp_device_has_feature (dev, FP_DEVICE_FEATURE_UPDATE_PRINT)) {
        printf ("El dispositivo soporta actualizaciones de huellas.\n");
        enroll_data->update_fingerprint = should_update_fingerprint ();
    }
    else {
        printf ("El dispositivo no soporta actualizaciones de huellas. Muestras antiguas serán borradas.\n");
        enroll_data->update_fingerprint = FALSE;
    }

    printf ("Es momento de enrolar tu huella.\n\n");
    printf ("Necesitarás escanear satisfactoriamente la huella de tu dedo %s "
            "%d veces para "
            "completar el proceso.\n\n", finger_to_string (enroll_data->_fingerprint.finger),
            fp_device_get_nr_enroll_stages (dev));
    printf ("Escanea tu huella ahora.\n");

    print_template = print_create_template (dev, enroll_data->_fingerprint.finger, enroll_data->update_fingerprint);
    fp_device_enroll (dev, print_template, enroll_data->_fingerprint._clear_storage.cancellable,
                    on_enroll_progress, NULL, NULL,
                    (GAsyncReadyCallback) on_enroll_completed,
                    enroll_data);
}

static gboolean sigint_cb (void *user_data) {
    EnrollData *enroll_data = user_data;

    g_cancellable_cancel (enroll_data->_fingerprint._clear_storage.cancellable);

    return G_SOURCE_CONTINUE;
}

int main () {
    g_autoptr(FpContext) ctx = NULL;
    g_autoptr(EnrollData) enroll_data = g_new0(EnrollData, 1);
    GPtrArray *devices;
    FpDevice *dev;
    FpFinger finger;

    g_print ("Este programa registrará el dedo seleccionado sobrescribiendo cualquier huella"
             " del mismo dedo que haya sido registrada anteriormente. Las actualizaciones de huellas"
             " sin borrar los datos anteriores son posibles en dispositivos que lo soporten. Ctrl+C interrumpe la ejecución del programa.\n");

    g_print ("Elige el dedo que deseas registrar:\n");
    finger = finger_chooser ();

    if (finger == FP_FINGER_UNKNOWN) {
        g_warning ("Se seleccionó un dedo desconocido");
        return EXIT_FAILURE;
    }

    setenv ("G_MESSAGES_DEBUG", "all", 0);

    ctx = fp_context_new ();

    devices = fp_context_get_devices (ctx);
    if (!devices) {
        g_warning ("No fue posible obtener los dispositivos");
        return EXIT_FAILURE;
    }

    dev = discover_device (devices);
    if (!dev) {
        g_warning ("No se detectaron dispositivos.");
        return EXIT_FAILURE;
    }

    enroll_data = g_new0 (EnrollData, 1);
    enroll_data->_fingerprint.finger = finger;
    enroll_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
    enroll_data->_fingerprint._clear_storage._session.loop = g_main_loop_new (NULL, FALSE);
    enroll_data->_fingerprint._clear_storage.cancellable = g_cancellable_new ();
    enroll_data->_fingerprint._clear_storage.sigint_handler = g_unix_signal_add_full (G_PRIORITY_HIGH,
                                                        SIGINT,
                                                        sigint_cb,
                                                        enroll_data,
                                                        NULL);

    fp_device_open (dev, enroll_data->_fingerprint._clear_storage.cancellable,
                    (GAsyncReadyCallback) on_device_opened,
                    enroll_data);

    g_main_loop_run (enroll_data->_fingerprint._clear_storage._session.loop);

    return enroll_data->_fingerprint._clear_storage._session.ret_value;
}
