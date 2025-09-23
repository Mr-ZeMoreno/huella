import ctypes
from enum import IntEnum

from pathlib import Path


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


current_path = Path(__file__).resolve().parent
fp = ctypes.CDLL(current_path / 'libfp_reader.so')

fp.fingerprint_init.restype = ctypes.c_void_p
fp.fingerprint_init.argtypes = []
fp.fingerprint_cleanup.restype = ctypes.c_void_p
fp.fingerprint_cleanup.argtypes = []

# Enroll
fp.fingerprint_enroll.restype = ctypes.c_int
fp.fingerprint_enroll.argtypes = [ctypes.c_char_p, ctypes.c_int]

def enroll(email: str, finger_number: FpFinger) -> int:

    fp.fingerprint_init()
    i = fp.fingerprint_enroll(email.encode('utf-8'), finger_number)
    fp.fingerprint_cleanup()

    return i


# Identify
fp.fingerprint_identify.restype = ctypes.c_char_p
fp.fingerprint_identify.argtypes = []

NO_IDENTIFICADO = "<NO IDENTIFICADO>"

def fp_init():
    return fp.fingerprint_init()

def fp_cleanup():
    return fp.fingerprint_cleanup()

def identify() -> str:
    return fp.fingerprint_identify().decode()

if __name__ == '__main__':
    email = "ejemplo@usm.cl"
    # enroll(email, FpFinger.INDICE_DERECHO)
    # verify(email)
    # print(f"El usuario es {identify()}")
