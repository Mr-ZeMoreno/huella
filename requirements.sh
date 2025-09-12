
#!/bin/bash

# Actualizar lista de paquetes
apt-get update

# Instalar dependencias necesarias para glib, gio, gobject, libfprint y json-glib


# Instalar cualquier otra dependencia adicional que tu proyecto necesite
# apt-get install -y <nombre_paquete>

# Limpieza de caché de apt-get para reducir el tamaño de la imagen
apt-get clean
rm -rf /var/lib/apt/lists/*

echo "Requerimientos instalados correctamente."
