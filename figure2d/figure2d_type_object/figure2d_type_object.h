#include <utils.h>
#include <memory>
#include <vector>
#include <forward_list>

class Figure2D;

class Figure2DType
{
protected:
    std::unique_ptr<Figure2DType> _parent {nullptr};
    float _x {0.0f};
    float _y {0.0f};

public:
    Figure2DType() {}

    Figure2DType(const float& x, const float& y)
        : _x(x), _y(y)
    {}

    Figure2DType(std::unique_ptr<Figure2DType>&& parent,
		 const float& x, const float& y)
        : _parent(std::move(parent)), _x(x), _y(y)
    {}

    virtual ~Figure2DType() {}

    std::unique_ptr<Figure2D> newFigure2D() {
	return std::make_unique<Figure2D>(*this);
    }

    float x() const
    {
	if (_x != 0.0f || _parent == nullptr)
            return _x;

	return _parent->x();
    }

    void setX(const float& x)
    {
        _x = x;
    }

    float y() const
    {
	if (_y != 0.0f || _parent == nullptr)
            return _y;

	return _parent->y();
    }

    void setY(const float& y)
    {
        _y = y;
    }

    virtual VertexArray draw()
    {
        return {};
    }
};

class Figure2D
{
    friend class Figure2DType;

    Figure2DType& _figureType;

public:
    Figure2D(Figure2DType& figureType)
        : _figureType(figureType)
    {}

    float x() const
    {
        return _figureType.x();
    }

    void setX(const float& x)
    {
        _figureType.setX(x);
    }

    float y() const
    {
        return _figureType.y();
    }

    void setY(const float& y)
    {
        _figureType.setY(y);
    }

    VertexArray draw()
    {
        return _figureType.draw();
    }
};

typedef glm::vec3 Color;
typedef std::vector<Color> Colors;

typedef glm::vec2 Vec2;
typedef std::vector<Vec2> Vertices;

class Triangle : public Figure2DType
{
    Vertex _startVertex { {0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} };
    Vertex _secondVertex { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} };
    Vertex _thirdVertex { {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} };

public:
    Triangle()
	: Figure2DType(_startVertex.pos.x, _startVertex.pos.y)
    {}

    Triangle(const float& startX, const float& startY,
             const float& secondX, const float& secondY,
             const float& thirdX, const float& thirdY)
        : Figure2DType(startX, startY)
    {
	_startVertex.pos = {startX, startY};
        _secondVertex.pos = {secondX, secondY};
	_thirdVertex.pos = {thirdX, thirdY};
    }

    Triangle(const Vec2& startVertex,
             const Vec2& secondVertex,
             const Vec2& thirdVertex)
        : Figure2DType(startVertex.x, startVertex.y)
    {
	_startVertex.pos = startVertex;
        _secondVertex.pos = secondVertex;
	_thirdVertex.pos = thirdVertex;
    }

    Colors colors() const
    {
	Color startColor = _startVertex.color;
	Color secondColor = _secondVertex.color;
        Color thirdColor = _thirdVertex.color;

        return {startColor, secondColor, thirdColor};
    }

    void setColors(const Color& startColor,
		   const Color& secondColor,
		   const Color& thirdColor)
    {
        _startVertex.color = startColor;
	_secondVertex.color = secondColor;
	_thirdVertex.color = thirdColor;
    }

    void setColors(const Colors& newColors)
    {
        _startVertex.color = newColors[0];
	_secondVertex.color = newColors[1];
	_thirdVertex.color = newColors[2];
    }

    Vertices vertices() const
    {
	Vec2 startPos = _startVertex.pos;
	Vec2 secondPos = _secondVertex.pos;
        Vec2 thirdPos = _thirdVertex.pos;

        return {startPos, secondPos, thirdPos};
    }

    void setVertices(const Vec2& startVertex,
                     const Vec2& secondVertex,
                     const Vec2& thirdVertex)
    {
	_startVertex.pos = startVertex;
        _secondVertex.pos = secondVertex;
        _thirdVertex.pos = thirdVertex;
    }

    void setVertices(const Vertices& newVertices)
    {
	_startVertex.pos = newVertices[0];
        _secondVertex.pos = newVertices[1];
        _thirdVertex.pos = newVertices[2];
    }

    VertexArray draw() override
    {
	return {_startVertex, _secondVertex, _thirdVertex};
    }
};

class Square : public Figure2DType
{
    float _side;

    Color _startColor {1.0f, 0.0f, 0.0f};
    Color _color2 {0.0f, 1.0f, 0.0f};
    Color _color3 {0.0f, 0.0f, 1.0f};
    Color _color4 {1.0f, 1.0f, 1.0f};

public:
    Square()
	: Figure2DType(-0.5f, -0.5f), _side(0.5f)
    {}

    Square(const float& startX, const float& startY, const float& side = 0.5f)
        : Figure2DType(startX, startY), _side(side)
    {}

    Square(const Vec2& startVertex, const float& side = 0.5f)
	: Figure2DType(startVertex.x, startVertex.y), _side(side)
    {}

    Colors colors() const
    {
        return {_startColor, _color2, _color3, _color4};
    }

    void setColors(const Color& startColor, const Color& color2,
	           const Color& color3, const Color& color4)
    {
        _startColor = startColor;
	_color2 = color2;
	_color3 = color3;
	_color3 = color4;
    }

    void setColors(const Colors& colors)
    {
        _startColor = colors[0];
	_color2 = colors[1];
	_color3 = colors[2];
	_color3 = colors[3];
    }

    Vec2 start() const
    {
	return {_x, _y};
    }

    void setStart(const Vec2& newStart) {
	_x = newStart.x;
	_y = newStart.y;
    }

    float side() const
    {
	return _side;
    }

    void setSide(const float& newSide) {
	_side = newSide;
    }

    VertexArray draw() override
    {
	Vertex startVertex {{_x, _y}, _startColor};
	Vertex vertex2 {{_x + _side, _y}, _color2};
	Vertex vertex3 {{-(_x + _side), -_y}, _color3};
	Vertex vertex4 {{_x, -_y}, _color4};

        return {startVertex, vertex2, vertex3, vertex4};
    }
};

class Circle : public Figure2DType
{
    float _radius {1.0f};

    uint8_t _triangleAmount = 40;
    const float twicePi = 2.0f * M_PI;

    typedef std::forward_list<Color> ColorList;

    ColorList vertexColors {
        {1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f},
	{1.0f, 1.0f, 1.0f}
    };

public:
    Circle() {}

    Circle(const float& centerX, const float& centerY, const float& radius)
        : Figure2DType(centerX, centerY), _radius(radius)
    {}

    Colors colors() const
    {
        return {vertexColors.begin(), vertexColors.end()};
    }

    void setColors(const Colors& newColors) {
	vertexColors.assign(newColors.begin(), newColors.end());
    }

    void setColors(const ColorList& newColors) {
        vertexColors = newColors;
    }

    Vec2 center() const
    {
	return {_x, _y};
    }

    void setCenter(const float& newCenterX, const float& newCenterY) {
	_x = newCenterX;
	_y = newCenterY;
    }

    void setCenter(const Vec2& newCenter) {
	_x = newCenter.x;
	_y = newCenter.y;
    }

    float radius() const
    {
        return _radius;
    }

    void setRadius(const float& newRadius) {
        _radius = newRadius;
    }

    VertexArray draw() override
    {
	auto colorsIter = vertexColors.cbegin();

        VertexArray vertexArray;
        vertexArray.emplace_back(Vertex{{_x, _y}, *colorsIter++});

        for (uint8_t i = 0; i < _triangleAmount; ++i) {
            float x = _x + _radius * cos(i * twicePi / _triangleAmount);
            float y = _y + _radius * sin(i * twicePi / _triangleAmount);

	    //float x = _x + _radius * cos(i / _triangleAmount * twicePi);
	    //float y = _y + _radius * sin(i / _triangleAmount * twicePi);

	    if (colorsIter == vertexColors.cend()) colorsIter = vertexColors.cbegin();

            vertexArray.emplace_back(Vertex{{x, y}, *colorsIter++});
        }

        return vertexArray;
    }
};
