from pyray import *
import asyncio
import websockets
import json

async def ws_send(ws, message):
	await ws.send(message)
	print("Sent:", message)

async def main():
	ws = await websockets.connect("ws://192.168.1.148/ws")
	motors = (0, 0)
	prev = (0, 0)
	OFFSET = 20

	def draw_motors():
		begin_drawing()
		clear_background(WHITE)
		draw_rectangle(OFFSET, OFFSET, 50, 255, GRAY)
		draw_rectangle(OFFSET, OFFSET, 50, abs(motors[0]), RED if motors[0] > 0 else BLUE)
		draw_rectangle(OFFSET*2+50, OFFSET, 50, 255, GRAY)
		draw_rectangle(OFFSET*2+50, OFFSET, 50, abs(motors[1]), RED if motors[1] > 0 else BLUE)
		end_drawing()
	
	def get_dir_axis(ax, ay):
		ax *= 255
		ay *= 255
		if ax != 0 and ay != 0:
			return ((ax+ay)//2, (-ax+ay)//2)
		if ax != 0:
			return (ax, -ax)
		if ay != 0:
			return (ay, ay)
		return (0, 0)
	
	def get_dir_joy():
		x = int(get_gamepad_axis_movement(0, GamepadAxis.GAMEPAD_AXIS_LEFT_X)) * 255
		y = -int(get_gamepad_axis_movement(0, GamepadAxis.GAMEPAD_AXIS_RIGHT_Y)) * 255
		return get_dir_axis(x, y)
	
	def get_dir_key():
		x = (-1 if is_key_down(KeyboardKey.KEY_LEFT) else 0) + (1 if is_key_down(KeyboardKey.KEY_RIGHT) else 0)
		y = (-1 if is_key_down(KeyboardKey.KEY_DOWN) else 0) + (1 if is_key_down(KeyboardKey.KEY_UP) else 0)
		
		return get_dir_axis(x, y)

	init_window(50*2+OFFSET*3, 255+OFFSET*2, "Hello")

	while not window_should_close():
		motors = get_dir_key()
		draw_motors()
		if motors != prev:
			await ws_send(ws, json.dumps({"action": "setspeed", "motor": 0, "speed": motors[1]}))
			await ws_send(ws, json.dumps({"action": "setspeed", "motor": 1, "speed": motors[0]}))
		prev = motors

	close_window()


if __name__ == "__main__":
	asyncio.run(main())