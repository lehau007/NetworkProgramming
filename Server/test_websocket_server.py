import asyncio
import websockets

async def communicate():
    uri = "ws://localhost:8000"
    
    async with websockets.connect(uri) as websocket:
        # # 1. Send a message
        # message = "Hello, WebSocket!"
        # await websocket.send(message)
        # print(f"Sent: {message}")

        for i in range(5):
            # 2. Wait for the response
            response = await websocket.recv()
            print(f"Received: {response}")
            
            message = f"Message {i + 1}"
            await websocket.send(message)
            print(f"Sent: {message}")

if __name__ == "__main__":
    asyncio.run(communicate())