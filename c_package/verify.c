/*
 * Example fingerprint verification program, which verifies the
 * finger which has been previously enrolled to disk.
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

 #include "fp-device.h"
 #include "fp-print.h"
 #include "gio/gio.h"
 #include "glibconfig.h"

 #include <stdio.h>
 #include <fprint.h>
 #include <glib-unix.h>

 #include "consts.h"

 #include "storage.h"
 #include "utilities.h"
 #include "verify.h"


 static void start_verification (FpDevice *dev, VerifyData *verify_data);

 static void on_verify_completed (FpDevice *dev, GAsyncResult *res, void *user_data) {
     VerifyData *verify_data = user_data;

     g_autoptr(FpPrint) print = NULL;
     g_autoptr(GError) error = NULL;
     gboolean match;

     if (!fp_device_verify_finish (dev, res, &match, &print, &error)) {
         g_warning ("Fallo al verificar la huella: %s", error->message);
         verify_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;

         if (error->domain != FP_DEVICE_RETRY) {
             fingerprint_quit (dev, (FingerprintSession*) verify_data);
             return;
         }
     }

     while(verify_data->retries > 0){
         start_verification(dev, verify_data);
         return;
     }


     fingerprint_quit (dev, (FingerprintSession*) verify_data);
 }

 static void on_match_cb (FpDevice *dev, FpPrint *match, FpPrint *print, gpointer user_data, GError *error);


 static FpPrint* get_stored_print (FpDevice *dev, VerifyData *verify_data) {
     FpPrint *verify_print;

     verify_print = print_data_load_from_json(verify_data->user_email, ENROLLED_JSON_PATH);

     verify_data->_fingerprint.finger = fp_print_get_finger(verify_print);

     g_print ("Cargando datos previamente guardados para el dedo %s...\n",
              finger_to_string (verify_data->_fingerprint.finger));

     return verify_print;
 }

 static void start_verification (FpDevice *dev, VerifyData *verify_data) {
     if (!verify_data->user_email) {
         g_warning("No se pudo obtener el correo electrónico.");
         return;
     }

     g_print("Correo electrónico ingresado: %s\n", verify_data->user_email);

     g_autoptr(FpPrint) verify_print = get_stored_print (dev, verify_data);

     if (!verify_print) {
         g_warning("No se pudo cargar la huella del usuario.");
         fingerprint_quit (dev, (FingerprintSession*) verify_data);
         return;
     }

     g_print ("Huella cargada. Procediendo a la verificación...\n");

     fp_device_verify(dev, verify_print, verify_data->_fingerprint._clear_storage.cancellable,
                      on_match_cb, verify_data, NULL, (GAsyncReadyCallback) on_verify_completed, verify_data);
 }

 static void on_device_opened (FpDevice *dev, GAsyncResult *res, void *user_data) {
     VerifyData *verify_data = user_data;

     fingerprint_device_open(dev, res);

     start_verification (dev, verify_data);
 }


 static void on_match_cb (FpDevice *dev, FpPrint *match, FpPrint *print, gpointer user_data, GError *error) {
     VerifyData *verify_data = user_data;

     if (error) {
         g_warning ("Huella no coincide. Error de reintento reportado: %s", error->message);
         return;
     }

     if (match) {
         const GDate *date = fp_print_get_enroll_date (match);
         char date_str[128] = "<desconocida>";
         verify_data->_fingerprint._clear_storage._session.ret_value = EXIT_SUCCESS;

         if (date && g_date_valid (date))
             g_date_strftime (date_str, G_N_ELEMENTS (date_str), "%Y-%m-%d\0", date);

         g_debug("Informe de coincidencia: el dispositivo %s coincidió con el dedo %s exitosamente "
                "usando la huella %s, registrada el %s por el usuario %s",
                fp_device_get_name (dev),
                finger_to_string(fp_print_get_finger (match)),
                fp_print_get_description (match),
                date_str,
                fp_print_get_username(match));

         g_print("HUELLA COINCIDE\n");
         verify_data->retries = 0;
     } else {
         g_debug("Informe de coincidencia: huella no coincide.");
         g_print ("HUELLA NO COINCIDE\n");
         g_print("Tienes %i intentos\n", verify_data->retries);
         verify_data->retries--;
     }
 }

 int verify(char* user_email, int retries) {
     VerifyData* verify_data = g_new(VerifyData, 1);
     g_autoptr(FpContext) ctx = NULL;
     GPtrArray *devices;
     FpDevice *dev;

     // setenv ("G_MESSAGES_DEBUG", "all", 0);
     // setenv ("LIBUSB_DEBUG", "3", 0);

     ctx = fp_context_new ();

     devices = fp_context_get_devices (ctx);
     if (!devices) {
         g_warning ("No se pudo obtener la lista de dispositivos.");
         return EXIT_FAILURE;
     }

     dev = discover_device (devices);
     if (!dev) {
         g_warning ("No se detectaron dispositivos.");
         return EXIT_FAILURE;
     }


     verify_data->user_email = g_strdup(user_email);
     verify_data->retries = retries;
     verify_data->_fingerprint._clear_storage._session.ret_value = EXIT_FAILURE;
     verify_data->_fingerprint._clear_storage._session.loop = g_main_loop_new(NULL, FALSE);
     verify_data->_fingerprint._clear_storage.cancellable = g_cancellable_new();
     verify_data->_fingerprint._clear_storage.sigint_handler = g_unix_signal_add_full(
         G_PRIORITY_HIGH, SIGINT, sigint_cb, verify_data, NULL);

     fp_device_open(dev, verify_data->_fingerprint._clear_storage.cancellable,
                    (GAsyncReadyCallback) on_device_opened, verify_data);

     g_main_loop_run(verify_data->_fingerprint._clear_storage._session.loop);

     int ret = verify_data->_fingerprint._clear_storage._session.ret_value;

     verify_data_free(verify_data);

     return ret;
 }
