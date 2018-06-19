#ifndef _VECTOR2_H_
#define _VECTOR2_H_

class Vector2
{
public:
	int x, y;

	Vector2()
	{
		x = 0;
		y = 0;
	}

	~Vector2() = default;

	Vector2(Vector2 const& in)
	{
		x = in.x;
		y = in.y;
	}

	Vector2(const int _x, const int _y)
	{
		x = _x;
		y = _y;
	}

	Vector2(Vector2& in)
	{
		x = in.x;
		y = in.y;
	}

	void Set(const int _x, const int _y)
	{
		x = _x;
		y = _y;
	}
};

#endif