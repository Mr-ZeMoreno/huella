/*
 * Trivial storage driver for example programs
 *
 * Copyright (C) 2019 Benjamin Berg <bberg@redhat.com>
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

#pragma once

#include <glib.h>
#include <fprint.h>

GPtrArray* gallery_data_load_from_json(const char *json_path);

int save_into_json_file(gchar* user_id, char* base64, const char* path);

FpPrint * print_create_template (FpDevice *dev, FpFinger finger);

FpPrint *print_data_load_from_json(const gchar *user_id, const gchar *json_path);

gboolean finger_exists(FpFinger finger, const char* json_path);

gboolean email_exists(const char* email, const char* json_path);

gboolean update_fingerprint_by_email(const gchar *email, FpPrint *new_print, const gchar *json_path);
