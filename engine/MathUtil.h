#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <iostream>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace ECSEngine
{
    struct Point2D
    {
        float x, y;
        Point2D(float x = 0, float y = 0)
            : x(x), y(y) {}

        float Distance(const Point2D &other) const
        {
            float dx = x - other.x;
            float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }
        Point2D operator+(const Point2D &other) const
        {
            return Point2D(x + other.x, y + other.y);
        }
        Point2D operator+(const float &other) const
        {
            return Point2D(x + other, y + other);
        }
        Point2D operator-(const Point2D &other) const
        {
            return Point2D(x - other.x, y - other.y);
        }
        Point2D operator-(const float &other) const
        {
            return Point2D(x - other, y - other);
        }
        Point2D operator*(const float &scalar) const
        {
            return Point2D(x * scalar, y * scalar);
        }
        Point2D &operator+=(const float &scalar)
        {
            x += scalar;
            y += scalar;
            return *this;
        }
        Point2D &operator+=(const Point2D &other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }
        Point2D &operator-=(const Point2D &other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        bool operator==(const Point2D &other) const
        {
            return x == other.x && y == other.y;
        }
        Point2D &operator*=(const int &scalar)
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }
        Point2D &operator/=(const int &scalar)
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }
        float operator*(const Point2D &other) const
        {
            return (x * other.x + y * other.y);
        }
        float Dot(Point2D b) const
        {
            return (x * b.x + y * b.y);
        }
        float Cross(Point2D b) const
        {
            return (x * b.y - y * b.x);
        }
        static float Dot(Point2D a, Point2D b)
        {
            return (a.x * b.x + a.y * b.y);
        }
        static float Cross(Point2D a, Point2D b) // not technically a cross product
        {
            return (a.x * b.y - a.y * b.x);
        }

        void Normalize()
        {
            float length = std::sqrt(x * x + y * y); // sqrt of (x^2 + y^2) / length
            if (length != 0)
            {
                x /= length;
                y /= length;
            }
        }
    };

    static std::ostream &operator<<(std::ostream &os, const Point2D &p)
    {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }

    struct Rect
    {
        Point2D topLeft;
        int width, height;

        Rect(float left, float top, float width, float height)
            : topLeft(Point2D(left, top)), width(width), height(height)
        {
        }
        Rect(Point2D tl = {0, 0}, int w = 0, int h = 0)
            : topLeft(tl), width(w), height(h)
        {
        }
        Rect(Point2D p1, Point2D p2)
            : topLeft(std::min(p1.x, p2.x), std::min(p1.y, p2.y)),
              width(fabs(p1.x - p2.x)), height(fabs(p1.y - p2.y))
        {
        }
        Rect(Point2D center, float radius)
            : topLeft(center.x - radius, center.y - radius),
              width(2 * radius), height(2 * radius)
        {
        }

        // Union will expand this rectangle to include the other rectangle or point
        Rect &operator|=(const Rect &other)
        {
            float newLeft = std::min(topLeft.x, other.topLeft.x);
            float newTop = std::min(topLeft.y, other.topLeft.y);
            float newRight = std::max(topLeft.x + width, other.topLeft.x + other.width);
            float newBottom = std::max(topLeft.y + height, other.topLeft.y + other.height);
            topLeft.x = newLeft;
            topLeft.y = newTop;
            width = newRight - newLeft;
            height = newBottom - newTop;
            return *this;
        }
        Rect &operator|=(const Point2D &other)
        {
            float newLeft = std::min(topLeft.x, other.x);
            float newTop = std::min(topLeft.y, other.y);
            float newRight = std::max(topLeft.x + width, other.x);
            float newBottom = std::max(topLeft.y + height, other.y);
            topLeft.x = newLeft;
            topLeft.y = newTop;
            width = newRight - newLeft;
            height = newBottom - newTop;
            return *this;
        }

        // Intersection will shrink this rectangle to the overlapping area with the other rectangle
        Rect &operator&=(const Rect &other)
        {
            float newLeft = std::max(topLeft.x, other.topLeft.x);
            float newTop = std::max(topLeft.y, other.topLeft.y);
            float newRight = std::min(topLeft.x + width, other.topLeft.x + other.width);
            float newBottom = std::min(topLeft.y + height, other.topLeft.y + other.height);
            if (newRight < newLeft || newBottom < newTop)
            {
                topLeft.x = 0;
                topLeft.y = 0;
                width = 0;
                height = 0;
            }
            else
            {
                topLeft.x = newLeft;
                topLeft.y = newTop;
                width = newRight - newLeft;
                height = newBottom - newTop;
            }
            return *this;
        }

        // Inset will shrink the rectangle by the given amount on all sides
        void Inset(int inset)
        {
            topLeft.x += inset;
            topLeft.y += inset;
            width -= 2 * inset;
            height -= 2 * inset;
            if (width < 0)
                width = 0;
            if (height < 0)
                height = 0;
        }

        bool IsInside(const Point2D &p) const
        {
            // Check if point is left, right, below, above
            return !(p.x < topLeft.x || p.x > topLeft.x + width || p.y < topLeft.y || p.y > topLeft.y + height);
        }

        bool intersects(const Rect &other) const
        {
            return !(topLeft.x + width < other.topLeft.x || other.topLeft.x + other.width < topLeft.x ||
                     topLeft.y + height < other.topLeft.y || other.topLeft.y + other.height < topLeft.y);
        }
    };

    inline Point2D Rotate(const Point2D &vector, float angle)
    {
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        return Point2D(
            vector.x * cos_a - vector.y * sin_a,
            vector.x * sin_a + vector.y * cos_a);
    }

    inline Point2D GetOverlap(const Rect &a, const Rect &b)
    {
        float aRight = a.topLeft.x + a.width;
        float aBottom = a.topLeft.y + a.height;
        float bRight = b.topLeft.x + b.width;
        float bBottom = b.topLeft.y + b.height;

        float overlapX = std::min(aRight, bRight) - std::max(a.topLeft.x, b.topLeft.x);
        float overlapY = std::min(aBottom, bBottom) - std::max(a.topLeft.y, b.topLeft.y);

        return Point2D(overlapX, overlapY);
    }
}

#endif
