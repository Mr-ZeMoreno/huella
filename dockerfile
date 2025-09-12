
FROM archlinux:latest

# Configura el directorio de trabajo
WORKDIR /app

# Copia los archivos del proyecto al contenedor
COPY *.c *.h *.build ./
COPY py_package/so ./py_package/so

# Instalar dependencias necesarias
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
    base-devel \
    git \
    meson \
    ninja \
    pkg-config \
    python3 \
    python-pip \
    libfprint \
    glib2 \
    json-glib && \
    pacman -Scc --noconfirm  # Limpieza de caché

COPY entrypoint.sh .

ENTRYPOINT [ "./entrypoint.sh" ]
