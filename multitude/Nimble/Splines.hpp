/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_SPLINES_HPP
#define NIMBLE_SPLINES_HPP

#include "Export.hpp"
#include "Vector2.hpp"

#include <vector>

namespace Luminous {
  class RenderContext;
}

namespace Nimble
{

  /// Evaluate a cubic Catmull-Rom spline on an interval.
  /// @param t interpolation parameter [0, 1]
  /// @param cp vector of control points (needs at least four)
  /// @param index to the first control point used in interpolation. Used
  ///        control points will be from [index, index + 3]
  /// @return value of the spline at given t
  template<class T>
  T evalCatmullRom(float t, const std::vector<T> & cp, size_t index = 0);

  /// Catmull-Rom
  /// @todo doc
  class NIMBLE_API Interpolating {
  public:
    /// Get derivative at the given interpolation point
    Nimble::Vector2 getDerivative(size_t ii, float t) const;
    /// Evaluates the spline at given t
    /// @param t position where to evaluate the spline. 0 <= t <= size() - 1
    Nimble::Vector2 get(float t) const;
    /// Adds a control point
    void add(Nimble::Vector2 point);
    /// Removes the control point at the given index
    void remove(size_t ii);
    /// Returns the number of control points
    size_t size() const { return m_points.size(); }

    /// Clears the interpolation key-points
    void clear();

    friend class Luminous::RenderContext;
  private:
    typedef std::vector<Nimble::Vector2> PointList;

    PointList m_points;
    PointList m_tangents;
    Nimble::Vector2 get(size_t ii, float h1, float h2, float h3, float h4) const;
    Nimble::Vector2 getPoint(size_t ii, float t) const;
  };

}

#endif
