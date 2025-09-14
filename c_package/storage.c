/*
 * Trivial storage driver for example programs
 *
 * Copyright (C) 2019 Benjamin Berg <bberg@redhat.com>
 * Copyright (C) 2019-2020 Marco Trevisan <marco.trevisan@canonical.com>
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

#include <fprint.h>
#include "storage.h"
#include "fp-print.h"
#include "glib.h"

#include <glib/gstdio.h>
#include <stdio.h>
#include <unistd.h>
#include <json-glib/json-glib.h>

#include <stdio.h>
#include <glib.h>

gboolean ensure_json_file_exists(const char *json_path) {
    g_return_val_if_fail(json_path != NULL, FALSE);

    FILE *file = fopen(json_path, "r");

    if (file) {
        fclose(file);
        return TRUE; // El archivo ya existe
    }

    file = fopen(json_path, "w");
    if (!file) {
        g_warning("No se pudo crear el archivo JSON: %s", json_path);
        return FALSE;
    }

    fputs("{}", file);
    fclose(file);

    g_message("Archivo JSON creado: %s", json_path);
    return FALSE; // Se tuvo que crear
}

int generate_json(JsonBuilder* builder, const char* path){
    JsonGenerator *generator = json_generator_new();
    JsonNode *root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);

    GError *error = NULL;
    if (!json_generator_to_file(generator, path, &error)) {
        g_warning("Error al guardar JSON en %s: %s", path, error->message);
        g_error_free(error);
        g_object_unref(generator);
        json_node_free(root);
        g_object_unref(builder);
        return -1;
    }

    g_object_unref(generator);
    g_object_unref(builder);

    return 0;
}

GPtrArray* gallery_data_load_from_json(const char *json_path) {
    g_autoptr(GError) error = NULL;
    g_autoptr(JsonParser) parser = json_parser_new();
    g_autoptr(GPtrArray) gallery = g_ptr_array_new_with_free_func(g_object_unref);

    if (!json_parser_load_from_file(parser, json_path, &error)) {
        g_warning("No se pudo cargar el archivo JSON: %s", error->message);
        return gallery;
    }

    JsonNode *root = json_parser_get_root(parser);

    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_warning("El JSON no es un objeto válido");
        return gallery;
    }

    JsonObject *root_obj = json_node_get_object(root);
    GList *members = json_object_get_members(root_obj);

    for (GList *l = members; l != NULL; l = l->next) {
        const gchar *email = l->data;
        const gchar *b64_data = json_object_get_string_member(root_obj, email);

        gsize bin_len;
        guchar *bin_data = g_base64_decode(b64_data, &bin_len);

        g_autoptr(GError) deser_error = NULL;
        FpPrint *print = fp_print_deserialize(bin_data, bin_len, &deser_error);
        g_free(bin_data);

        if (!print) {
            g_warning("No se pudo deserializar la huella para %s: %s", email, deser_error->message);
            continue;
        }

        fp_print_set_username(print, email);  // útil si luego lo necesitas
        g_ptr_array_add(gallery, print);
    }

    g_list_free(members);
    return g_steal_pointer(&gallery);
}


int save_into_json_file(const char* user_id, char* base64, const char* path) {
    g_return_val_if_fail(user_id != NULL, -1);
    g_return_val_if_fail(base64 != NULL, -1);
    g_return_val_if_fail(path != NULL, -1);

    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);

    // Escribe el usuario directamente como clave
    json_builder_set_member_name(builder, user_id);
    json_builder_add_string_value(builder, base64);

    json_builder_end_object(builder);

    return generate_json(builder, path);
}



FpPrint *print_data_load_from_json(const gchar *user_id, const gchar *json_path) {
    g_return_val_if_fail(user_id != NULL, NULL);
    g_return_val_if_fail(json_path != NULL, NULL);

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonParser) parser = json_parser_new();

    if (!json_parser_load_from_file(parser, json_path, &error)) {
        g_warning("No se pudo cargar el archivo JSON '%s': %s", json_path, error->message);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_warning("El archivo JSON no contiene un objeto válido en la raíz");
        return NULL;
    }

    JsonObject *root_obj = json_node_get_object(root);
    if (!json_object_has_member(root_obj, user_id)) {
        g_warning("No hay huella registrada para el usuario '%s'", user_id);
        return NULL;
    }

    const gchar *b64_data = json_object_get_string_member(root_obj, user_id);
    if (!b64_data || *b64_data == '\0') {
        g_warning("El usuario '%s' tiene datos vacíos", user_id);
        return NULL;
    }

    gsize bin_len = 0;
    guchar *bin_data = g_base64_decode(b64_data, &bin_len);

    if (!bin_data || bin_len == 0) {
        g_warning("Fallo al decodificar los datos base64 para el usuario '%s'", user_id);
        return NULL;
    }

    g_autoptr(GError) deser_error = NULL;
    FpPrint *print = fp_print_deserialize(bin_data, bin_len, &deser_error);
    g_free(bin_data);

    if (!print) {
        g_warning("Fallo al deserializar la huella para '%s': %s", user_id,
                  deser_error ? deser_error->message : "error desconocido");
        return NULL;
    }

    fp_print_set_username(print, user_id);  // opcional

    return print;  // el caller debe hacer g_object_unref()
}

gboolean finger_exists(FpFinger finger, const char* json_path) {
    g_return_val_if_fail(json_path != NULL, FALSE);

    // Asegurar que el archivo existe antes de intentar cargarlo
    ensure_json_file_exists(json_path);

    GPtrArray *gallery = gallery_data_load_from_json(json_path);
    if (!gallery) {
        g_warning("La galería retornada es NULL");
        return FALSE;
    }

    if (gallery->len == 0) {
        g_message("La galería está vacía");
        return FALSE;
    }

    for (guint i = 0; i < gallery->len; i++) {
        FpPrint *print = g_ptr_array_index(gallery, i);
        if (fp_print_get_finger(print) == finger) {
            return TRUE;  // Ya hay una huella para este dedo
        }
    }

    return FALSE;
}

gboolean email_exists(const char* email, const char* json_path) {
    g_return_val_if_fail(email != NULL, FALSE);
    g_return_val_if_fail(json_path != NULL, FALSE);

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonParser) parser = json_parser_new();

    if (!json_parser_load_from_file(parser, json_path, &error)) {
        // Si no existe el archivo, asumimos que el email no existe aún
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        return FALSE;
    }

    JsonObject *root_obj = json_node_get_object(root);
    return json_object_has_member(root_obj, email);
}

gboolean update_fingerprint_by_email(const gchar *email, FpPrint *new_print, const gchar *json_path) {
    g_return_val_if_fail(email != NULL, FALSE);
    g_return_val_if_fail(new_print != NULL, FALSE);
    g_return_val_if_fail(json_path != NULL, FALSE);

    g_autoptr(GError) error = NULL;
    g_autoptr(JsonParser) parser = json_parser_new();

    // Asegurar que el archivo JSON existe
    if (!ensure_json_file_exists(json_path)) {
        g_warning("El archivo JSON no existe y no se pudo crear.");
        return FALSE;
    }

    // Cargar el archivo JSON
    if (!json_parser_load_from_file(parser, json_path, &error)) {
        g_warning("No se pudo cargar el archivo JSON: %s", error->message);
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_warning("El JSON no contiene un objeto válido");
        return FALSE;
    }

    JsonObject *root_obj = json_node_get_object(root);

    // Verificar que el email exista antes de actualizar
    if (!json_object_has_member(root_obj, email)) {
        g_warning("No existe una huella registrada para el correo '%s'.", email);
        return FALSE;
    }

    // Serializar la nueva huella
    guchar *serialized_data = NULL;
    gsize serialized_len = 0;

    if (!fp_print_serialize(new_print, &serialized_data, &serialized_len, &error)) {
        g_warning("Error al serializar la huella: %s", error->message);
        return FALSE;
    }

    // Convertir a base64
    gchar *b64_data = g_base64_encode(serialized_data, serialized_len);
    g_free(serialized_data);

    // Reemplazar el valor correspondiente al email
    json_object_set_string_member(root_obj, email, b64_data);
    g_free(b64_data);

    // Escribir el JSON de vuelta al archivo
    g_autoptr(JsonGenerator) generator = json_generator_new();
    json_generator_set_root(generator, root);

    if (!json_generator_to_file(generator, json_path, &error)) {
        g_warning("No se pudo guardar el archivo JSON actualizado: %s", error->message);
        return FALSE;
    }

    g_message("Huella actualizada correctamente para '%s'", email);
    return TRUE;
}
