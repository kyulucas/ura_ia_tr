from fastapi import APIRouter, WebSocket

router = APIRouter()

@router.get("/health")
def health_check():
    return {"status": "ok"}

@router.websocket("/ws/call/{call_id}")
async def websocket_call(websocket: WebSocket, call_id: str):
    await websocket.accept()
    await websocket.send_text(f"WebSocket ativo para chamada {call_id}")
    await websocket.close() 