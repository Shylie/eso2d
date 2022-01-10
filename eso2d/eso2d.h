#pragma once

#include <cstdint>
#include <utility>
#include <vector>

/// <summary>
/// Print code into the terminal at (x, y).
/// </summary>
/// <param name="x">X position to print at.</param>
/// <param name="y">Y position to print at.</param>
/// <param name="code">Code to print.</param>
extern void Put(int x, int y, int code);
/// <summary>
/// Set draw order, with higher numbers drawn later.
/// </summary>
/// <param name="layer">Draw order. Higher is later.</param>
extern void Layer(int layer);
/// <summary>
/// Sets terminal foreground color, if applicable. Do nothing otherwise.
/// </summary>
/// <param name="color">Color to set the foreground to.</param>
extern void SetColor(uint32_t color);
/// <summary>
/// Make a color from RGBA components.
/// </summary>
/// <param name="r">Red component.</param>
/// <param name="g">Green component.</param>
/// <param name="b">Blue component.</param>
/// <param name="a">Alpha component.</param>
/// <returns>The color.</returns>
extern uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

namespace OpCode
{
	enum OpCode : int
	{
		None = ' ', // empty cell
		Path = '.', // used for determining ip path,
		Skip = ':', // skip a cell
		Left = 'l', // l - move selection left
		Right = 'r', // r - move selection right
		Up = 'u', // u - move selection up
		Down = 'd', // d - move selection down
		Widen = 'w', // widen selection cursor
		Shrink = 's', // shrink selection cursor
		Move = 'm', // move data under selection from old selection position to current selection position
		Terminate = '#', // kill cursor. if all cursors are dead, stop program execution
		IPStart = '@', // ip starts at this location
		SelectionStart = '_' // selection starts at this location
	};
}

class Selection
{
public:
	Selection();
	Selection(int x, int y);

	int X() const;
	int Y() const;
	int PreviousX() const;
	int PreviousY() const;

	void Print(const class Grid&) const;

	void SetPosition(int x, int y, const class Grid&);
	void MoveBy(int dx, int dy, const class Grid&);

private:
	int x;
	int y;
	int prevX;
	int prevY;
};

class WSelection : public Selection
{
public:
	WSelection();
	WSelection(int x, int y);

	int Width() const;

	void Print(const class Grid&) const;

	void Widen(const class Grid&);
	void Shrink(const class Grid&);

private:
	int width;
};

class Cursor
{
public:
	Cursor();
	Cursor(int ipx, int ipy, int sx, int sy);

	/// <summary>
	/// Print the cursor to the terminal.
	/// </summary>
	/// <param name="grid">Grid containing the code.</param>
	void Print(const class Grid& grid) const;

	/// <summary>
	/// Execute one instruction.
	/// </summary>
	/// <param name="grid">Grid containing the code.</param>
	/// <returns>True if the cursor is still alive, false otherwise.</returns>
	bool Update(class Grid& grid);

private:
	Selection ip;
	WSelection selected;

	int dx;
	int dy;

	void Move(class Grid&);
	void TurnLeft();
	void TurnRight();
};

class Grid
{
public:
	class View;
	class ConstView;

	friend void swap(Grid& first, Grid& second) noexcept;

	Grid(int w, int h);

	Grid(const Grid&);
	Grid(Grid&&) noexcept;

	Grid& operator=(const Grid&);
	Grid& operator=(Grid&&) noexcept;

	~Grid();

	int& operator()(int x, int y);
	int operator()(int x, int y) const;
	int& operator()(Selection selection, bool previous = false);
	int operator()(Selection selection, bool previous = false) const;
	View operator()(WSelection selection, bool previous = false);
	ConstView operator()(WSelection selection, bool previous = false) const;

	int Width() const;
	int Height() const;

	void Print() const;

	bool Update();

	void AddCursor(int ipX, int ipY, int selX, int selY);
	void Stop();

private:
	int width;
	int height;
	int* gridData;

	std::vector<Cursor> cursors;

	class View
	{
	public:
		View(Grid* grid, int x, int y, int width);

		int& operator()(int offset);
		int operator()(int offset) const;

	private:
		Grid* grid;
		int x;
		int y;
		int width;
	};

	class ConstView
	{
	public:
		ConstView(const Grid* grid, int x, int y, int width);

		int operator()(int offset) const;

	private:
		const Grid* grid;
		int x;
		int y;
		int width;
	};

	Grid();
};