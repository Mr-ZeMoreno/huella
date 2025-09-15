import asyncio
from concurrent.futures import ThreadPoolExecutor
from fastapi import FastAPI
from http import HTTPStatus
from fingerprint import enroll , FpFinger, identify, NO_IDENTIFICADO


app = FastAPI()

executor = ThreadPoolExecutor(max_workers=1)

async def async_function(function, *args):
    loop = asyncio.get_event_loop()
    return await loop.run_in_executor(executor, function, *args)

@app.get("/enroll/{email}")
async def enroll_user(email: str, finger:FpFinger = FpFinger.INDICE_DERECHO):
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

@app.get("/identify")
async def identify_user():
    ret = await async_function(identify)

    if ret.lower() != NO_IDENTIFICADO.lower():
        return {"status": HTTPStatus.OK, "user": ret}
    elif ret.lower() == NO_IDENTIFICADO.lower():
        return {"status": HTTPStatus.BAD_REQUEST, "message": "Usuario no identificado"}
    else:
        return {"status": HTTPStatus.INTERNAL_SERVER_ERROR, "message": "Failed to identify user"}

# @app.get("/verify/{email}")
# async def verify_user(email: str):
#     ret = await async_function(verify, email, 1)
#     if ret == 0:
#         return {"status": HTTPStatus.OK, "message": "Usuario identificado"}
#     else:
#         return {"status": HTTPStatus.INTERNAL_SERVER_ERROR, "message": "Failed to verify user"}
