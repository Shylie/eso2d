#include "eso2d.h"
#include "BearLibTerminal.h"

void Put(int x, int y, int code)
{
	terminal_put(x, y, code);
}

void Layer(int layer)
{
	terminal_layer(layer);
}

void SetColor(uint32_t color)
{
	terminal_color(color);
}

uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return color_from_argb(a, r, g, b);
}

int main()
{
	terminal_open();
	terminal_set("window.title='eso2d'");
	terminal_set("input.filter={keyboard, mouse}");
	terminal_refresh();

	const int w = terminal_state(TK_WIDTH);
	const int h = terminal_state(TK_HEIGHT);
	Grid grid(w, h);

	int x = 0;
	int y = 0;

	grid.Print();
	terminal_color(0xFFFF0000);
	terminal_layer(2);
	terminal_put(x, y, '_');
	terminal_refresh();

	bool loop = true;
	while (loop)
	{
		terminal_clear();

		{
			int code = terminal_read();
			if (code == TK_MOUSE_LEFT)
			{
				x = terminal_state(TK_MOUSE_X);
				y = terminal_state(TK_MOUSE_Y);
			}
			else if (code == TK_LEFT)
			{
				if (--x < 0) { x = 0; }
			}
			else if (code == TK_RIGHT)
			{
				if (++x >= grid.Width()) { x--; }
			}
			else if (code == TK_UP)
			{
				if (--y < 0) { y = 0; }
			}
			else if (code == TK_DOWN)
			{
				if (++y >= grid.Height()) { y--; }
			}
			else if (code == TK_CLOSE)
			{
				loop = false;
			}
		}

		if (terminal_state(TK_ENTER))
		{
			int ipStartX = -1;
			int ipStartY = -1;
			int selStartX = -1;
			int selStartY = -1;
			for (int i = 0; i < grid.Width(); i++)
			{
				for (int j = 0; j < grid.Height(); j++)
				{
					switch (grid(i, j))
					{
					case OpCode::IPStart:
						ipStartX = i;
						ipStartY = j;
						break;

					case OpCode::SelectionStart:
						selStartX = i;
						selStartY = j;
						break;
					}
				}
			}

			if (ipStartX >= 0 && ipStartY >= 0 && selStartX >= 0 && selStartY >= 0)
			{
				grid.AddCursor(ipStartX, ipStartY, selStartX, selStartY);
				do
				{
					terminal_clear();
					grid.Print();
					terminal_refresh();
					if (terminal_has_input())
					{
						int code = terminal_read();
						if (code == TK_CLOSE)
						{
							loop = false;
							break;
						}
						else if (code == TK_ESCAPE)
						{
							break;
						}
					}
					terminal_delay(200);
				}
				while (grid.Update());

				while (terminal_has_input())
				{
					terminal_read();
				}

				terminal_clear();
				grid.Print();

				terminal_color(0xFFFF0000);
				terminal_layer(2);
				terminal_put(x, y, '_');
				terminal_refresh();
				continue;
			}
		}

		int inputChar = terminal_state(TK_CHAR);
		if (inputChar)
		{
			grid(x, y) = inputChar;
			if (++x >= grid.Width()) { x--; }
		}
		else if (terminal_state(TK_BACKSPACE))
		{
			grid(x, y) = OpCode::None;
			if (--x < 0) { x = 0; }
		}

		grid.Print();

		terminal_color(0xFFFF0000);
		terminal_layer(2);
		terminal_put(x, y, '_');

		terminal_refresh();
	}

	terminal_close();

	return 0;
}