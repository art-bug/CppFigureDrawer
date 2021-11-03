#include <vector>
#include <memory>

struct Vertex2f
{
    float x {0.0f};
    float y {0.0f};

    Vertex2f() {}

    Vertex2f(const float& x, const float& y)
        : x(x), y(y)
    {}

    Vertex2f(const Vertex2f& vertex2f)
	: x(vertex2f.x), y(vertex2f.y)
    {}
};

typedef std::vector<std::unique_ptr<Vertex2f>> VertexBuffer;

class Figure2D
{
protected:
    float _x {0.0f};
    float _y {0.0f};

public:
    Figure2D() {}

    Figure2D(const float& x, const float& y)
        : _x(x), _y(y)
    {}

    virtual ~Figure2D() {}

    float& x() const
    {
        return _x;
    }

    void setX(const float& x)
    {
        _x = x;
    }

    float& y() const
    {
        return _y;
    }

    void setY(const float& y)
    {
        _y = y;
    }

    virtual VertexBuffer draw() const
    {
        return {};
    }
};

class Triangle : public Figure2D
{
    Vertex2f _secondVertex;
    Vertex2f _thirdVertex;

public:
    Triangle() {}

    Triangle(const float& startX, const float& startY,
             const float& secondX, const float& secondY,
             const float& thirdX, const float& thirdY)
        : Figure2D(startX, startY),
	  _secondVertex(secondX, secondY), _thirdVertex(thirdX, thirdY)
    {}

    VertexBuffer draw() override
    {
        VertexBuffer vertexBuffer;
	vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x, _y));
	vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_secondVertex);
	vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_thirdVertex);

	return vertexBuffer;
    }
};

class Square : public Figure2D
{
    float _side {0.0f};

public:
    Square() {}

    Square(const float& startX, const float& startY, const float& side)
        : Figure2D(startX, startY), _side(side)
    {}

    VertexBuffer draw() override
    {
        VertexBuffer vertexBuffer;
        vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x, _y));
        vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x + _side, _y));
        vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x + _side, _y - _side));
        vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x, _y - _side));

        return vertexBuffer;
    }
}

class Circle : public Figure2D
{
    float _radius {0.0f};

    uint8_t _triangleAmount = 40;
    const float twicePi = 2.0f * M_PI;

public:
    Circle() {}

    Circle(const float& centerX, const float& centerY, const float& radius)
        : Figure2D(centerX, centerY), _radius(radius)
    {}

    VertexBuffer draw() override
    {
        VertexBuffer vertexBuffer;
        vertexBuffer.emplace_back(std::make_unique<Vertex2f>(_x, _y));

        for (uint8_t i = 0; i < _triangleAmount; ++i) {
            float x = _x + _radius * cos(i * twicePi / _triangleAmount);
            float y = _y + _radius * sin(i * twicePi / _triangleAmount);

            vertexBuffer.emplace_back(std::make_unique<Vertex2f>(x, y));
        }

        return vertexBuffer;
    }
};
