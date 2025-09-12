import ctypes
from enum import IntEnum

class FpFinger(IntEnum):
    DESCONOCIDO = 0
    PULGAR_DERECHO = 6
    INDICE_DERECHO = 7
    MEDIO_DERECHO = 8
    ANULAR_DERECHO = 9
    MENIQUE_DERECHO = 10
    PULGAR_IZQUIERDO = 1
    INDICE_IZQUIERDO = 2
    MEDIO_IZQUIERDO = 3
    ANULAR_IZQUIERDO = 4
    MENIQUE_IZQUIERDO = 5


# Carga la librería (ruta relativa o absoluta)
fp = ctypes.CDLL('../build/libfp_reader.so')

# Enroll
fp.enroll.restype = ctypes.c_int
fp.enroll.argtypes = [ctypes.c_char_p, ctypes.c_int]

def enroll(email: str, finger_number: FpFinger) -> int:
    return fp.enroll(email.encode('utf-8'), finger_number)

# Verify
fp.verify.restype = ctypes.c_int
fp.verify.argtypes = [ctypes.c_char_p, ctypes.c_int]

def verify(email:str, retries:int=3) -> int:
    return fp.verify(email.encode('utf-8'), retries)

# Identify
fp.identify.restype = ctypes.c_char_p
fp.identify.argtypes = []

def identify() -> str:
    return fp.identify().decode()


if __name__ == '__main__':
    email = "ejemplo@usm.cl"
    # enroll(email, FpFinger.INDICE_DERECHO)
    # verify(email)
    print(f"El usuario es {identify()}")
