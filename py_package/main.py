import asyncio
from concurrent.futures import ThreadPoolExecutor
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from http import HTTPStatus
from fingerprint import enroll , FpFinger, identify, NO_IDENTIFICADO, fp_init, fp_cleanup
from pydantic import EmailStr
import json

app = FastAPI()

# app.add_middleware(
#     CORSMiddleware,
#     allow_origins=["*"]
# )

executor = ThreadPoolExecutor(max_workers=2)

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

# @app.get("/identify")
# async def identify_user():
#     for attempt in range(1, MAX_IDENTIFY_ATTEMPTS + 1):
#         ret = await async_function(identify)

#         if ret.lower() != NO_IDENTIFICADO.lower():
#             return {
#                 "status": HTTPStatus.OK,
#                 "user": ret,
#                 "attempt": attempt
#             }

#     # Si llega aquí, no se identificó ningún usuario
#     return {
#         "status": HTTPStatus.BAD_REQUEST,
#         "message": f"Usuario no identificado tras {MAX_IDENTIFY_ATTEMPTS} intentos"
#     }


# Crear un semáforo global
semaforo_ws = asyncio.Semaphore(1)
disconnected_event = asyncio.Event()

@app.websocket("/ws/identify")
async def websocket_identify(websocket: WebSocket):
    await websocket.accept()

    # Intentamos adquirir el semáforo antes de procesar la solicitud
    async with semaforo_ws:
        MAX_ATTEMPTS = 5
        attempt = 1

        # Restablecer el evento de desconexión antes de empezar
        disconnected_event.clear()

        try:
            while attempt <= MAX_ATTEMPTS:
                # Verificar si el cliente está desconectado antes de cada intento
                if disconnected_event.is_set():
                    await websocket.send_text("🚫 El cliente se ha desconectado.")
                    break

                fp_init()

                # Esperar un mensaje del cliente
                action = await websocket.receive_text()

                try:
                    action_data = json.loads(action)  # Suponemos que el cliente envía JSON
                    command = action_data.get("action")

                    if command == "identify":
                        await websocket.send_text(f"Intento {attempt} de {MAX_ATTEMPTS}...")
                        ret = identify()  # Llamada sincrónica

                        if ret.lower() != "no identificado":
                            await websocket.send_text(json.dumps({"user": ret}))
                            break  # Salir si el usuario es identificado

                        await websocket.send_text("❌ No se detectó huella, intenta nuevamente...")
                        attempt += 1

                    elif command == "cancel_scan":
                        await websocket.send_text("🛑 Proceso de escaneo cancelado.")
                        break

                    else:
                        await websocket.send_text("❌ Comando no reconocido.")

                except json.JSONDecodeError:
                    await websocket.send_text("❌ Error en el formato del mensaje.")

                fp_cleanup()

            if attempt > MAX_ATTEMPTS:
                fp_cleanup()
                await websocket.send_text("🚫 Usuario no identificado tras múltiples intentos.")
                raise Exception("Usuario no identificado")

        except WebSocketDisconnect:
            print("🔌 Cliente desconectado del WebSocket")
            disconnected_event.set()  # Marcar que el cliente se desconectó
            fp_cleanup()

        finally:
            fp_cleanup()
            # Asegúrate de cerrar la conexión aquí o manejar reconexiones desde el cliente.
            await websocket.close()
