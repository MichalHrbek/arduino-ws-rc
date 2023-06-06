using System.Threading;
using Websocket.Client;
using Newtonsoft.Json;
using Raylib_cs;
using System.Numerics;

// See https://aka.ms/new-console-template for more information

Console.WriteLine("Hello, World!");
NotifyPacket np = null;

const int OFFSET = 20;
try
{
	var exitEvent = new ManualResetEvent(false);
	var url = new Uri("ws://192.168.1.148/ws");

	using (var client = new WebsocketClient(url))
	{
		client.ReconnectTimeout = TimeSpan.FromSeconds(5);
		client.ReconnectionHappened.Subscribe(info => { Console.WriteLine("Reconnection happened, type: " + info.Type); });
		client.MessageReceived.Subscribe(msg =>
		{
			Console.WriteLine("Message received: " + msg);
			np = JsonConvert.DeserializeObject<NotifyPacket>(msg.Text);
		});
		await client.Start();
		loop(client);
	}
}
catch (Exception ex)
{
	Console.WriteLine("ERROR: " + ex.ToString());
}

void loop(WebsocketClient client)
{
	Raylib.InitWindow(50*2+OFFSET*5+255, 255+OFFSET*2, "Hello World");

	var gyroCube = Raylib.LoadModelFromMesh(Raylib.GenMeshCube(1f, 1f, 1f));

	(int, int) old_power = (0,0);
	while (!Raylib.WindowShouldClose())
	{
		Raylib.BeginDrawing();
		Raylib.ClearBackground(Color.WHITE);

		var power = getMotorPower(getDirKey());
		drawMotors(power);
		
		if (power != old_power)
		{
			Console.WriteLine(power.Item1);
			client.Send(JsonConvert.SerializeObject(new MotorPacket(0, power.Item1)));
			client.Send(JsonConvert.SerializeObject(new MotorPacket(1, power.Item2)));
			old_power = power;
		}

		if (np != null)
		{
			int w = 50;
			int h = 50;
			int x = OFFSET*3+50;
			int y = OFFSET;
			Raylib.DrawRectanglePro(new Rectangle(x, y, w, h), Vector2.Zero, np.gyro.X*36f, Color.GREEN);
			//Raylib.DrawRectangle(, )
		}

		Raylib.EndDrawing();
	}
	Raylib.CloseWindow();
}

void drawMotors((int, int) motors)
{
	Raylib.DrawRectangle(OFFSET, OFFSET, 50, 255, Color.GRAY);
	Raylib.DrawRectangle(OFFSET, OFFSET, 50, Math.Abs(motors.Item1), motors.Item1 > 0 ? Color.RED : Color.BLUE);
	Raylib.DrawRectangle(OFFSET*2+50, OFFSET, 50, 255, Color.GRAY);
	Raylib.DrawRectangle(OFFSET*2+50, OFFSET, 50, Math.Abs(motors.Item2), motors.Item2 > 0 ? Color.RED : Color.BLUE);
}

(int, int) getDirJoy()
{
	int x = (int)Raylib.GetGamepadAxisMovement(0, GamepadAxis.GAMEPAD_AXIS_LEFT_X) * 255;
	int y = (int)-Raylib.GetGamepadAxisMovement(0, GamepadAxis.GAMEPAD_AXIS_RIGHT_Y) * 255;
	return (x, y);
}

(int, int) getDirKey()
{
	int x = ((Raylib.IsKeyDown(KeyboardKey.KEY_RIGHT) ? 1 : 0) - (Raylib.IsKeyDown(KeyboardKey.KEY_LEFT) ? 1 : 0)) * 255;
	int y = ((Raylib.IsKeyDown(KeyboardKey.KEY_UP) ? 1 : 0) - (Raylib.IsKeyDown(KeyboardKey.KEY_DOWN) ? 1 : 0)) * 255;
	return (x, y);
}

(int, int) getMotorPower((int, int) axis)
{
	if (axis.Item1 == 0) return (axis.Item2, axis.Item2);
	return (-axis.Item1, axis.Item1);
}


class MotorPacket
{
	public string action = "setspeed";
	public int motor;
	public int speed;
	public MotorPacket(int motor, int speed)
	{
		this.motor = motor;
		this.speed = speed;
	}
}

class NotifyPacket
{
	public double temp;
	public double mpu_temp;
	public bool ic_error;
	public Vector3 gyro;
	public Vector3 acceleration;
}