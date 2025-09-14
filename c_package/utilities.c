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
// #include "gio/gio.h"
// #include "glib-object.h"
// #include "glib.h"
// #include "glibconfig.h"

#include <fprint.h>
#include <stdio.h>

#include "utilities.h"



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

FpPrint* print_create_template (FpDevice *dev, FpFinger finger) {
    g_autoptr(GVariantDict) dict = NULL;
    g_autoptr(GDateTime) datetime = NULL;
    g_autoptr(GDate) date = NULL;
    g_autoptr(GVariant) existing_val = NULL;
    FpPrint *template = NULL;
    gint year, month, day;

    if (template == NULL) {
        template = fp_print_new (dev);
        fp_print_set_finger (template, finger);
        fp_print_set_username (template, g_get_user_name ());
    }

    datetime = g_date_time_new_now_local ();
    g_date_time_get_ymd (datetime, &year, &month, &day);
    date = g_date_new_dmy (day, month, year);
    fp_print_set_enroll_date (template, date);

    return template;
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
