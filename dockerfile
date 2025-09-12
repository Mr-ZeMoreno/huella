# Utiliza la imagen base ARMv7 si la Raspberry Pi tiene un procesador ARMv7
FROM dockcross/linux-armv7:latest

# Establecer el directorio de trabajo
WORKDIR /app

# Copiar el script requirements.sh
# COPY requirements.sh .

# Actualizar los repositorios y la lista de paquetes
RUN apt-get update && apt-get upgrade -y

# Instalar las dependencias necesarias (actualizar Python y las bibliotecas)
RUN apt-get install -y \
    libglib2.0-dev \
    libgio-2.0-dev \
    libfprint-2-dev \
    libjson-glib-dev \
    meson \
    ninja-build \
    build-essential \
    pkg-config \
    python3 \
    python3-pip \
    python3-setuptools

# Copiar los archivos de código fuente y el archivo meson.build
COPY *.c *.h meson.build ./

# Ejecutar meson para configurar la construcción en el directorio 'build'
RUN meson setup build

RUN mkdir so

# Copiar el script compile.sh
COPY compile.sh .

# Ejecutar meson y ninja para compilar el proyecto
CMD ["./compile.sh"]
