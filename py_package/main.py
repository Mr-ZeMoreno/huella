import asyncio
from concurrent.futures import ThreadPoolExecutor
from fastapi import FastAPI
from http import HTTPStatus
from fingerprint import enroll , FpFinger, identify, NO_IDENTIFICADO
from pydantic import EmailStr

app = FastAPI()

executor = ThreadPoolExecutor(max_workers=1)

async def async_function(function, *args):
    loop = asyncio.get_event_loop()
    return await loop.run_in_executor(executor, function, *args)

@app.get("/enroll/{email}")
async def enroll_user(email: EmailStr, finger:FpFinger = FpFinger.INDICE_DERECHO):
    ret = await async_function(enroll, email, finger)

    if ret == 0:
        return {"status": HTTPStatus.OK, "message": "User enrolled successfully"}
    else:
        return {"status": HTTPStatus.INTERNAL_SERVER_ERROR, "message": "Failed to enroll user"}

@app.get("/fingers")
def list_fingers():
    return [
        {"name": finger.name, "value": finger.value}
        for finger in FpFinger
    ]




MAX_IDENTIFY_ATTEMPTS = 5

@app.get("/identify")
async def identify_user():
    for attempt in range(1, MAX_IDENTIFY_ATTEMPTS + 1):
        ret = await async_function(identify)

        if ret.lower() != NO_IDENTIFICADO.lower():
            return {
                "status": HTTPStatus.OK,
                "user": ret,
                "attempt": attempt
            }

    # Si llega aquí, no se identificó ningún usuario
    return {
        "status": HTTPStatus.BAD_REQUEST,
        "message": f"Usuario no identificado tras {MAX_IDENTIFY_ATTEMPTS} intentos"
    }
