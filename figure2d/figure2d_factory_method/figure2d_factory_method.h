#include "..\figure2d.h"
#include <memory>

struct Figure2DFactory
{
    virtual std::unique_ptr<Figure2D> makeFigure() = 0;
};

struct CircleFactory : Figure2DFactory
{
    std:unique_ptr<Figure2D> makeFigure()
    {
        return std::make_unique<Circle>();
    }
};

struct SquareFactory : Figure2DFactory
{
    std:unique_ptr<Figure2D> makeFigure()
    {
        return std::make_unique<Square>();
    }
};
