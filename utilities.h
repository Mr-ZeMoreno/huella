/*
 * Trivial storage driver for example programs
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

 #pragma once

#include "gio/gio.h"
#include "glibconfig.h"
#include <libfprint-2/fprint.h>
 #include <glib.h>
 #include <glib-unix.h>

 typedef struct {
     GMainLoop *loop;
     int ret_value;
 } Session;

 typedef struct {
     Session _session;
     GCancellable *cancellable;
     unsigned int sigint_handler;
 } ClearStorageData;

 typedef struct {
     ClearStorageData _clear_storage;
     FpFinger finger;
 } FingerprintSession;

 typedef struct {
     FingerprintSession _fingerprint;
 } VerifyData;

 typedef struct {
     FingerprintSession _fingerprint;
     gboolean update_fingerprint;
     char* base64;
     char* user_email;
 } EnrollData;

 typedef struct {
     FingerprintSession _fingerprint;
 } IdentifyData;

 typedef struct {
     FingerprintSession _fingerprint;
     GList *to_delete;
     gboolean any_failed;
 } ListData;

 typedef struct {
     ListData *list_data;
     FpPrint *print;
 } DeleteData;

// Funciones utilitarias
FpDevice * discover_device (GPtrArray *devices);
FpFinger finger_chooser (void);
const char * finger_to_string (FpFinger finger);

// Callback genérico para cerrar dispositivo
void fingerprint_device_closed(FpDevice* dev, GAsyncResult* res, void* user_data);
void clear_storage_device_closed(FpDevice *dev, GAsyncResult *res, void *user_data);

void list_data_free(ListData* session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ListData, list_data_free)

void enroll_data_free(EnrollData* session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(EnrollData, enroll_data_free)

void verify_data_free(VerifyData* session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(VerifyData, verify_data_free)

void clear_storage_data_free(ClearStorageData *session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(ClearStorageData, clear_storage_data_free)

void delete_data_free(DeleteData* session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(DeleteData, delete_data_free)


void identify_data_free (IdentifyData *session);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (IdentifyData, identify_data_free)

gboolean sigint_cb(void* session);

int save_data_into_member(FpPrint* print, char** base64);
int input_user_email(char **user_email);
