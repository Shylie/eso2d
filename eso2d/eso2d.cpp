#include "eso2d.h"

#include <algorithm>

#include <cassert>

static int Wrap(int a, int b)
{
	if (a < 0)
	{
		return b - (b - a) % b;
	}
	else
	{
		return a % b;
	}
}

Selection::Selection() : Selection(0, 0) { }
Selection::Selection(int x, int y) : x(x), y(y), prevX(x), prevY(y), wrappedX(false), wrappedY(false) { }

int Selection::X() const { return x; }
int Selection::Y() const { return y; }
int Selection::PreviousX() const { return prevX; }
int Selection::PreviousY() const { return prevY; }
bool Selection::MovedLeft() const
{
	return wrappedX ? x > prevX : x < prevX;
}
bool Selection::MovedRight() const
{
	return wrappedX ? x < prevX : x > prevX;
}
bool Selection::MovedUp() const
{
	return wrappedY ? y < prevY : y > prevY;
}
bool Selection::MovedDown() const
{
	return wrappedY ? y > prevY : y < prevY;
}

void Selection::Print(const Grid& grid) const
{
	SetColor(MakeColor(0xFF, 0x99, 0x00, 0xFF));
	Put(x, y, '_');
}

void Selection::SetPosition(int x, int y, const Grid& grid)
{
	wrappedX = wrappedY = false;

	if (x < 0 || x >= grid.Width())
	{
		x = Wrap(x, grid.Width());
		wrappedX = true;
	}

	if (y < 0 || y >= grid.Height())
	{
		y = Wrap(y, grid.Height());
		wrappedY = true;
	}

	prevX = this->x;
	prevY = this->y;

	this->x = x;
	this->y = y;
}

void Selection::MoveBy(int dx, int dy, const Grid& grid)
{
	SetPosition(x + dx, y + dy, grid);
}

WSelection::WSelection() : WSelection(0, 0, 1) { }
WSelection::WSelection(int x, int y, int w) : Selection(x, y), width(w < 1 ? 1 : w) { }

int WSelection::Width() const { return width; }

void WSelection::Print(const Grid& grid) const
{
	SetColor(MakeColor(0xFF, 0x44, 0x00, 0xFF));
	Put(X(), Y(), '_');
	SetColor(MakeColor(0xFF, 0x99, 0x00, 0xFF));
	int offset = 1;
	for (int i = 1; i < width; i++, offset++)
	{
		if (X() + offset >= grid.Width()) { offset -= grid.Width(); }
		Put(X() + offset, Y(), '_');
	}
}

void WSelection::Widen(const Grid& grid)
{
	if (width < grid.Width()) { width++; }
}
void WSelection::Shrink(const Grid& grid)
{
	if (width > 1) { width--; }
}

Cursor::Cursor() : Cursor(0, 0, 0, 0) { }
Cursor::Cursor(int ipx, int ipy, int sx, int sy) : Cursor(ipx, ipy, sx, sy, 1, 1, 0) { }
Cursor::Cursor(int ipx, int ipy, int sx, int sy, int sw, int dx, int dy) : ip(ipx, ipy), selected(sx, sy, sw), dx(dx), dy(dy) { }

void Cursor::Print(const Grid& grid) const
{
	ip.Print(grid);
	selected.Print(grid);
}

enum class Side
{
	None,
	Left,
	All,
	Right
};

bool Cursor::Update(Grid& grid)
{
	int instruction = grid(ip);
	Side side = Side::None;

	switch (instruction)
	{
	case OpCode::IPStart:
	case OpCode::Path:
		break;

	case OpCode::Skip:
		ip.MoveBy(dx, dy, grid);
		break;

	case OpCode::Left:
		selected.MoveBy(-1, 0, grid);
		break;

	case OpCode::Right:
		selected.MoveBy(1, 0, grid);
		break;

	case OpCode::Up:
		selected.MoveBy(0, -1, grid);
		break;

	case OpCode::Down:
		selected.MoveBy(0, 1, grid);
		break;

	case OpCode::Widen:
		selected.Widen(grid);
		break;

	case OpCode::Shrink:
		selected.Shrink(grid);
		break;

	case OpCode::Move:
		if (selected.MovedRight())
		{
			// moving right, iterate from right-to-left
			for (int i = selected.Width() - 1; i >= 0; i--)
			{
				grid(selected)(i) = grid(selected, true)(i);
			}
		}
		else if (selected.MovedLeft() || selected.Y() != selected.PreviousY())
		{
			// moving left, iterate from left-to-right
			// moving up or down, iteration order doesn't matter, memory will not overlap
			for (int i = 0; i < selected.Width(); i++)
			{
				grid(selected)(i) = grid(selected, true)(i);
			}
		}
		break;

	case OpCode::Increment:
	{
		int value = 0; // sum
		int placeValue = 1;
		bool validOperation = true;
		for (int i = selected.Width() - 1; i >= 0; i--)
		{
			// check if grid state is a number
			int gridValue = grid(selected)(i);
			if (gridValue < '0' || gridValue > '9') { validOperation = false; }

			value += (gridValue - '0') * placeValue; // digit * place value
			placeValue *= 10; // increment place value every iteration
		}

		// do nothing if not a valid number
		if (!validOperation) { break; }
		
		value += 1; // increment
		
		placeValue = 1;
		
		for (int i = selected.Width() - 1; i >= 0; i--)
		{
			// divide by place value to make single digit (also truncates unneeded digits due to integer type)
			// add to '0' to map to character range
			grid(selected)(i) = '0' + (value % (placeValue * 10) / placeValue);
			placeValue *= 10;
		}
		break;
	}

	case OpCode::Decrement:
	{
		int value = 0; // sum
		int placeValue = 1;
		bool validOperation = true;
		for (int i = selected.Width() - 1; i >= 0; i--)
		{
			// check if grid state is a number
			int gridValue = grid(selected)(i);
			if (gridValue < '0' || gridValue > '9') { validOperation = false; }

			value += (gridValue - '0') * placeValue; // digit * place value
			placeValue *= 10; // increment place value every iteration
		}

		// do nothing if not a valid number
		if (!validOperation) { break; }

		value -= 1;
		if (value < 0) { value = 0; } // no negative number support in this esolang
		
		placeValue = 1;
		
		for (int i = selected.Width() - 1; i >= 0; i--)
		{
			// divide by place value to make single digit (also truncates unneeded digits due to integer type)
			grid(selected)(i) = '0' + (value % (placeValue * 10) / placeValue);
			placeValue *= 10; // increment place value every iteration
		}
		break;
	}

	case OpCode::Set:
		ip.MoveBy(dx, dy, grid);
		for (int i = 0; i < selected.Width(); i++)
		{
			grid(selected)(i) = grid(ip);
		}
		break;

	case OpCode::Conditional:
	{
		bool equal = true;
		ip.MoveBy(dx, dy, grid);
		int gridValue = grid(ip);
		switch (gridValue)
		{
		case 'N':
			for (int i = 0; i < selected.Width(); i++)
			{
				if (grid(selected)(i) < '0' || grid(selected)(i) > '9')
				{
					equal = false;
					break;
				}
			}
			break;

		default:
			for (int i = 0; i < selected.Width(); i++)
			{
				if (gridValue != grid(selected)(i))
				{
					equal = false;
					break;
				}
			}
			break;
		}
		if (equal)
		{
			TurnLeft();
		}
		else
		{
			TurnRight();
		}
		break;
	}

	case OpCode::Split:
	{
		Cursor other(*this);
		other.TurnLeft();
		other.Move(grid);
		TurnRight();
		grid.QueueAddCursor(other);
		break;
	}

	case OpCode::LeftIndicator:
		side = Side::Left;
		break;

	case OpCode::RightIndicator:
		side = Side::Right;
		break;

	default: // OpCode::Terminate is also covered here
		return false;
	}

	Move(grid);

	if (side != Side::None)
	{
		instruction = grid(ip);
		int& target = side == Side::Left ? grid(selected)(0) : grid(selected)(selected.Width() - 1);
		switch (instruction)
		{
		case OpCode::Conditional:
			ip.MoveBy(dx, dy, grid);
			switch (grid(ip))
			{
			case 'W':
				if (side == Side::Right)
				{
					if (selected.Width() == grid.Width())
					{
						TurnLeft();
					}
					else
					{
						TurnRight();
					}
				}
				else
				{
					if (selected.Width() == 1)
					{
						TurnLeft();
					}
					else
					{
						TurnRight();
					}
				}
				break;

			case 'N':
				if (target >= '0' && target <= '9')
				{
					TurnLeft();
				}
				else
				{
					TurnRight();
				}
				break;

			default:
				if (grid(ip) == target)
				{
					TurnLeft();
				}
				else
				{
					TurnRight();
				}
				break;
			}
			Move(grid);
			break;

		case OpCode::Set:
			ip.MoveBy(dx, dy, grid);
			target = grid(ip);
			Move(grid);
			break;

		default:
			return false;
		}
	}

	return true;
}

void Cursor::Move(Grid& grid)
{
	ip.MoveBy(dx, dy, grid);

	int count = 0;
	int oppositeDX = -dx;
	int oppositeDY = -dy;
	
	// turn up to 4 times.
	// turn again if facing opposite direction (avoids turning around rather than left).
	while ((grid(ip) == OpCode::None || (dx == oppositeDX && dy == oppositeDY)) && count++ < 4)
	{
		ip.MoveBy(-dx, -dy, grid);
		TurnRight();
		ip.MoveBy(dx, dy, grid);
	}
}

void Cursor::TurnLeft()
{
	// currently moving on x-axis
	if (dx != 0)
	{
		dy = -dx;
		dx = 0;
	}
	// currently moving on y-axis
	else
	{
		dx = dy;
		dy = 0;
	}
}

void Cursor::TurnRight()
{
	// currently moving on x-axis
	if (dx != 0)
	{
		dy = dx;
		dx = 0;
	}
	// currently moving on y-axis
	else
	{
		dx = -dy;
		dy = 0;
	}
}

void swap(Grid& first, Grid& second) noexcept
{
	using std::swap;

	swap(first.width, second.width);
	swap(first.height, second.height);
	swap(first.gridData, second.gridData);
	swap(first.cursors, second.cursors);
}

std::ostream& operator<<(std::ostream& out, const Grid& grid)
{
	out << grid.width << std::endl;
	out << grid.height << std::endl;
	for (int i = 0; i < grid.width; i++)
	{
		for (int j = 0; j < grid.height; j++)
		{
			out << grid(i, j) << std::endl;
		}
	}
	return out;
}

std::istream& operator>>(std::istream& in, Grid& grid)
{
	int w, h;
	in >> w;
	in >> h;
	Grid tmp(w, h);
	for (int i = 0; i < grid.width; i++)
	{
		for (int j = 0; j < grid.height; j++)
		{
			in >> tmp(i, j);
		}
	}
	grid = std::move(tmp);
	return in;
}

Grid::Grid() : width(0), height(0), gridData(nullptr) { }
Grid::Grid(int w, int h) : width(w), height(h), gridData(new int[w * h])
{
	assert(w > 0 && h > 0);
	std::fill(gridData, gridData + width * height, OpCode::None);
}

Grid::Grid(const Grid& other) : width(other.width), height(other.height), gridData(new int[other.width * other.height]), cursors(other.cursors)
{
	std::copy(other.gridData, other.gridData + other.width * other.height, gridData);
}
Grid::Grid(Grid&& other) noexcept : Grid()
{
	swap(*this, other);
}

Grid& Grid::operator=(const Grid& other)
{
	Grid temp(other);
	swap(*this, temp);

	return *this;
}
Grid& Grid::operator=(Grid&& other) noexcept
{
	Grid temp(std::move(other));
	swap(*this, temp);
	return *this;
}

Grid::~Grid()
{
	delete[] gridData;
	gridData = nullptr;
}

int& Grid::operator()(int x, int y)
{
	assert(x >= 0 && y >= 0 && x < width && y < height);
	return gridData[x + y * width];
}

int Grid::operator()(int x, int y) const
{
	assert(x >= 0 && y >= 0 && x < width && y < height);
	return gridData[x + y * width];
}

int& Grid::operator()(Selection selection, bool previous)
{
	return (*this)(previous ? selection.PreviousX() : selection.X(), previous ? selection.PreviousY() : selection.Y());
}

int Grid::operator()(Selection selection, bool previous) const
{
	return (*this)(previous ? selection.PreviousX() : selection.X(), previous ? selection.PreviousY() : selection.Y());
}

Grid::View Grid::operator()(WSelection selection, bool previous)
{
	return View(this, previous ? selection.PreviousX() : selection.X(), previous ? selection.PreviousY() : selection.Y(), selection.Width());
}

Grid::ConstView Grid::operator()(WSelection selection, bool previous) const
{
	return ConstView(this, previous ? selection.PreviousX() : selection.X(), previous ? selection.PreviousY() : selection.Y(), selection.Width());
}

int Grid::Width() const { return width; }
int Grid::Height() const { return height; }

void Grid::Print() const
{
	SetColor(MakeColor(0xFF, 0xFF, 0xFF, 0xFF));
	Layer(0);
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			Put(i, j, (*this)(i, j));
		}
	}

	Layer(1);
	for (const Cursor& cursor : cursors)
	{
		cursor.Print(*this);
	}
}

bool Grid::Update()
{
	for (int i = cursors.size() - 1; i >= 0; i--)
	{
		if (!cursors[i].Update(*this))
		{
			cursors.erase(cursors.begin() + i);
		}
	}

	return cursors.size() > 0;
}

void Grid::QueueAddCursor(int ipX, int ipY, int selX, int selY)
{
	cursorsToAdd.emplace_back(ipX, ipY, selX, selY);
}

void Grid::QueueAddCursor(const Cursor& cursor)
{
	cursorsToAdd.push_back(cursor);
}

void Grid::AddCursors()
{
	while (!cursorsToAdd.empty())
	{
		cursors.push_back(cursorsToAdd.back());
		cursorsToAdd.pop_back();
	}
}

void Grid::Stop()
{
	cursors.clear();
}

Grid::View::View(Grid* grid, int x, int y, int width) : grid(grid), x(x), y(y), width(width) { }

int& Grid::View::operator()(int offset)
{
	assert(offset >= 0 && offset < width);
	return (*grid)((x + offset) % grid->Width(), y);
}
int Grid::View::operator()(int offset) const
{
	assert(offset >= 0 && offset < width);
	return (*grid)((x + offset) % grid->Width(), y);
}

Grid::ConstView::ConstView(const Grid* grid, int x, int y, int width) : grid(grid), x(x), y(y), width(width) { }

int Grid::ConstView::operator()(int offset) const
{
	assert(offset >= 0 && offset < width);
	return (*grid)((x + offset) % grid->Width(), y);
}