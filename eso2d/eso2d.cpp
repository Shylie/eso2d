#include "eso2d.h"

#include <algorithm>

#include <cassert>

Selection::Selection() : Selection(0, 0) { }
Selection::Selection(int x, int y) : x(x), y(y), prevX(x), prevY(y) { }

int Selection::X() const { return x; }
int Selection::Y() const { return y; }
int Selection::PreviousX() const { return prevX; }
int Selection::PreviousY() const { return prevY; }

void Selection::Print(const Grid& grid) const
{
	SetColor(MakeColor(0xFF, 0x99, 0x00, 0xFF));
	Put(x, y, '_');
}

void Selection::SetPosition(int x, int y, const Grid& grid)
{
	while (x < 0)
	{
		x += grid.Width();
	}
	while (x >= grid.Width()) { x -= grid.Width(); }

	while (y < 0) { y += grid.Height(); }
	while (y >= grid.Height()) { y -= grid.Height(); }

	prevX = this->x;
	prevY = this->y;

	this->x = x;
	this->y = y;
}

void Selection::MoveBy(int dx, int dy, const Grid& grid)
{
	SetPosition(x + dx, y + dy, grid);
}

WSelection::WSelection() : WSelection(0, 0) { }
WSelection::WSelection(int x, int y) : Selection(x, y), width(1) { }

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
Cursor::Cursor(int ipx, int ipy, int sx, int sy) : ip(ipx, ipy), selected(sx, sy), dx(1), dy(0) { }

void Cursor::Print(const Grid& grid) const
{
	ip.Print(grid);
	selected.Print(grid);
}

bool Cursor::Update(Grid& grid)
{
	int instruction = grid(ip);

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
		if (selected.X() > selected.PreviousX())
		{
			// moving right, iterate from right-to-left
			for (int i = selected.Width() - 1; i >= 0; i--)
			{
				grid(selected)(i) = grid(selected, true)(i);
			}
			grid(selected, true)(0) = OpCode::None;
		}
		else if (selected.X() < selected.PreviousX())
		{
			// moving left, up, or down, iterate from left-to-right
			for (int i = 0; i < selected.Width(); i++)
			{
				grid(selected)(i) = grid(selected, true)(i);
			}
			grid(selected, true)(selected.Width() - 1) = OpCode::None;
		}
		else if (selected.Y() != selected.PreviousY())
		{
			// copy data
			for (int i = 0; i < selected.Width(); i++)
			{
				grid(selected)(i) = grid(selected, true)(i);
			}
			// erase where the data was moved from
			for (int i = 0; i < selected.Width(); i++)
			{
				grid(selected, true)(i) = OpCode::None;
			}
		}
		break;

	default: // OpCode::Terminate is also covered here
		return false;
	}

	Move(grid);

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
	other.~Grid();
}

Grid& Grid::operator=(const Grid& other)
{
	Grid temp(other);
	swap(*this, temp);

	return *this;
}
Grid& Grid::operator=(Grid&& other) noexcept
{
	swap(*this, other);
	other.~Grid();
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
			cursors.pop_back();
		}
	}

	return cursors.size() > 0;
}

void Grid::AddCursor(int ipX, int ipY, int selX, int selY)
{
	cursors.emplace_back(ipX, ipY, selX, selY);
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