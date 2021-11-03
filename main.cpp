#include "figuredrawapplication/figuredrawapplication.h"

std::unique_ptr<Figure2D> getTriangle(const std::vector<glm::vec2>& vertices = {},
	                              const Colors& colors = {})
{
    auto triangle = std::make_unique<Triangle>();

    if (vertices.empty() && colors.empty())
        return triangle->newFigure2D();

    if (!vertices.empty()) triangle->setVertices(vertices);

    if (!colors.empty()) triangle->setColors(colors);

    return triangle->newFigure2D();
}

std::unique_ptr<Figure2D> getSquare(const glm::vec2& start = glm::vec2(0),
	                            const float& side = 0.0f,
                                    const Colors& colors = {})
{
    auto square = std::make_unique<Square>();

    if (start == glm::vec2(0) && side == 0.0f && colors.empty())
        return square->newFigure2D();

    if (start != glm::vec2(0)) square->setStart(start);

    if (side > 0.0f) square->setSide(side);

    if (!colors.empty()) square->setColors(colors);

    return square->newFigure2D();
}

std::unique_ptr<Figure2D> getCircle(const glm::vec2& center = glm::vec2(0),
	                            const float& radius = 0.0f,
	                            const Colors& colors = {})
{
    auto circle = std::make_unique<Circle>();

    if (center == glm::vec2(0) && radius == 0.0f && colors.empty())
        return circle->newFigure2D();

    if (center != glm::vec2(0)) circle->setCenter(center);

    if (radius > 0.0f) circle->setRadius(radius);

    if (!colors.empty()) circle->setColors(colors);

    return circle->newFigure2D();
}

int main() {
    FigureDrawApplication app;

    auto leftTriangle = std::make_unique<Triangle>(-0.75f, -0.5f,
		                                   -0.55f, 0.5f,
		                                   -0.95f, 0.5f);
    auto centerSquare = std::make_unique<Square>();
    auto rightCircle = std::make_unique<Circle>(0.55f, 0.0f, 0.42f);

    app.draw(leftTriangle->newFigure2D());
    app.draw(centerSquare->newFigure2D(), {0, 1, 2, 2, 3, 0});
    app.draw(rightCircle->newFigure2D());

    return app.run();
}
