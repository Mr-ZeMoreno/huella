/*
 *
 * Copyrights (c) 2025 Josecarlos Vidal <jvidalq@usm.cl>
 *
 * Esta biblioteca es software libre; usted puede redistribuirla y/o
 * modificarla bajo los términos de la Licencia Pública Menor de GNU
 * tal como la publica la Free Software Foundation; ya sea
 * la versión 2.1 de la Licencia, o (a su elección) cualquier versión posterior.
 *
 * Esta biblioteca se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTÍA; ni siquiera la garantía implícita de
 * COMERCIALIZACIÓN o IDONEIDAD PARA UN PROPÓSITO PARTICULAR. Consulte la
 * Licencia Pública Menor de GNU para más detalles.
 *
 * Usted debe haber recibido una copia de la Licencia Pública Menor de GNU
 * junto con esta biblioteca; si no es así, escriba a la Free Software Foundation, Inc.,
 * 51 Franklin Street, Quinto Piso, Boston, MA 02110-1301, USA.
 */

#include <fprint.h>
#include <stdio.h>

#include "utilities.h"
#include "fp-print.h"
#include <json-glib/json-glib.h>

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
