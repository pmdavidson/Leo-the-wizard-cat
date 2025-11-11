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

    struct Line
    {
        Point2D p1, p2;

        Line(Point2D p1 = {0, 0}, Point2D p2 = {0, 0})
            : p1(p1), p2(p2) {}

        Line(float x1, float y1, float x2, float y2)
            : p1(x1, y1), p2(x2, y2) {}

        float Length() const
        {
            return p1.Distance(p2);
        }

        Point2D ClosestPoint(const Point2D &p) const
        {
            Point2D ab = p2 - p1;
            Point2D ap = p - p1;
            float ab_len2 = ab.x * ab.x + ab.y * ab.y;

            if (ab_len2 == 0)
                return p1; // degenerate line

            float t = (ap.x * ab.x + ap.y * ab.y) / ab_len2;
            t = std::max(0.0f, std::min(1.0f, t)); // clamp to [0, 1]

            return p1 + ab * t;
        }

        bool Crosses(Line other, Point2D &crossingPoint) const
        {
            float x1 = p1.x, y1 = p1.y;
            float x2 = p2.x, y2 = p2.y;
            float x3 = other.p1.x, y3 = other.p1.y;
            float x4 = other.p2.x, y4 = other.p2.y;

            float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (fabs(denom) < 1e-6)
                return false; // parallel or coincident

            float px = ((x1 * y2 - y1 * x2) * (x3 - x4) -
                        (x1 - x2) * (x3 * y4 - y3 * x4)) /
                       denom;
            float py = ((x1 * y2 - y1 * x2) * (y3 - y4) -
                        (y1 - y2) * (x3 * y4 - y3 * x4)) /
                       denom;
            crossingPoint = {px, py};

            auto within = [](float a, float b, float c)
            {
                return (c >= std::min(a, b) - 1e-6 && c <= std::max(a, b) + 1e-6);
            };

            return within(x1, x2, px) && within(y1, y2, py) &&
                   within(x3, x4, px) && within(y3, y4, py);
        }
    };

    std::ostream &operator<<(std::ostream &os, const Line &l);

    struct Circle
    {
        Point2D center;
        float radius;

        Circle(Point2D c = {0, 0}, float r = 0)
            : center(c), radius(r) {}

        Circle(float x, float y, float r)
            : center(x, y), radius(r) {}
    };

    static std::ostream &operator<<(std::ostream &os, const Circle &c);

    inline float DegToRad(float degrees)
    {
        return degrees * M_PI / 180.0f;
    }

    inline Point2D Rotate(const Point2D &vector, float angle)
    {
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        return Point2D(
            vector.x * cos_a - vector.y * sin_a,
            vector.x * sin_a + vector.y * cos_a);
    }

    // For Engine collisions
    // Line and line is up top

    // inline bool LineIntersectsRect(const Line &line, const Rect &rect, Point2D *outPoint = nullptr) {
    //     // Get rectangle corners
    //     Point2D tl = rect.topLeft;
    //     Point2D tr(tl.x + rect.width, tl.y);
    //     Point2D bl(tl.x, tl.y + rect.height);
    //     Point2D br(tl.x + rect.width, tl.y + rect.height);

    //     Line edges[] = { {tl, tr}, {tr, br}, {br, bl}, {bl, tl} };

    //     for (const Line &edge : edges) {
    //         Point2D p;
    //         if (line.Crosses(edge, p)) {
    //             if (outPoint) *outPoint = p;
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    inline bool LineIntersectsCircle(const Line &line, const Circle &circle, Point2D *outPoint = nullptr)
    {
        std::cout << "[DEBUG] LineIntersectsCircle: Line(" << line.p1 << ", " << line.p2 << ") ";
        std::cout << "vs Circle(center=" << circle.center << ", r=" << circle.radius << ")\n";

        Point2D closest = line.ClosestPoint(circle.center);
        float dist = closest.Distance(circle.center);

        std::cout << "[DEBUG] LineIntersectsCircle CALLED with Line("
                  << line.p1 << ", " << line.p2 << ") vs Circle(center="
                  << circle.center << ", r=" << circle.radius << ")\n";

        if (dist <= circle.radius)
        {
            if (outPoint)
                *outPoint = closest;
            std::cout << "[DEBUG] Result = " << "true" << "\n";

            return true;
        }
        std::cout << "[DEBUG] Result = " << "false" << "\n";

        return false;
    }

    inline bool LineIntersectsRect(const Line &line, const Rect &rect, Point2D *outPoint = nullptr)
    {
        // handle case where line starts or ends inside the rect
        if (rect.IsInside(line.p1))
        {
            if (outPoint)
                *outPoint = line.p1;
            return true;
        }
        if (rect.IsInside(line.p2))
        {
            if (outPoint)
                *outPoint = line.p2;
            return true;
        }

        // Then do edge check
        Point2D tl = rect.topLeft;
        Point2D tr(tl.x + rect.width, tl.y);
        Point2D bl(tl.x, tl.y + rect.height);
        Point2D br(tl.x + rect.width, tl.y + rect.height);

        Line edges[] = {{tl, tr}, {tr, br}, {br, bl}, {bl, tl}};

        for (const Line &edge : edges)
        {
            Point2D p;
            if (line.Crosses(edge, p))
            {
                if (outPoint)
                    *outPoint = p;
                return true;
            }
        }
        return false;
    }

    inline bool CircleIntersectsRect(const Circle &circle, const Rect &rect, Point2D *outPoint = nullptr)
    {
        float closestX = std::clamp(circle.center.x, rect.topLeft.x, rect.topLeft.x + rect.width);
        float closestY = std::clamp(circle.center.y, rect.topLeft.y, rect.topLeft.y + rect.height);

        float dx = circle.center.x - closestX;
        float dy = circle.center.y - closestY;
        float distSq = dx * dx + dy * dy;

        if (distSq <= (circle.radius * circle.radius))
        {
            if (outPoint)
                *outPoint = Point2D(closestX, closestY);
            return true;
        }
        return false;
    }

    inline bool CircleIntersectsCircle(const Circle &a, const Circle &b, Point2D *outPoint = nullptr)
    {
        float dist = a.center.Distance(b.center);
        if (dist <= (a.radius + b.radius))
        {
            if (outPoint)
            {
                // Calculate collision point as the point on circle B's edge closest to A
                // This makes explosions chain backwards toward the shooter
                Point2D dir = a.center - b.center;
                dir.Normalize();
                *outPoint = b.center + dir * b.radius;
            }
            return true;
        }
        return false;
    }
}

#endif
