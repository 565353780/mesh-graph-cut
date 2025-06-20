/***************************************************************************
 *  This file is part of the MCUT project, which is comprised of a library 
 *  for surface mesh cutting, example programs and test programs.
 * 
 *  Copyright (C) 2024 CutDigital Enterprise Ltd
 *  
 *  MCUT is dual-licensed software that is available under an Open Source 
 *  license as well as a commercial license. The Open Source license is the 
 *  GNU Lesser General Public License v3+ (LGPL). The commercial license 
 *  option is for users that wish to use MCUT in their products for commercial 
 *  purposes but do not wish to release their software under the LGPL. 
 *  Email <contact@cut-digital.com> for further information.
 *
 *  You may not use this file except in compliance with the License. A copy of 
 *  the Open Source license can be obtained from
 *
 *      https://www.gnu.org/licenses/lgpl-3.0.en.html.
 *
 *  For your convenience, a copy of this License has been included in this
 *  repository.
 *
 *  MCUT is distributed in the hope that it will be useful, but THE SOFTWARE IS 
 *  PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 *  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR 
 *  A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 *  OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author(s):
 *
 *    Floyd M. Chitalu    CutDigital Enterprise Ltd.
 *
 **************************************************************************/

#ifndef MCUT_MATH_H_
#define MCUT_MATH_H_

#include <cmath>

#include <iostream>
#include <limits>
#include <memory>

#include <vector>

#include "mcut/internal/utils.h"

// Shewchuk predicates : shewchuk.c
extern "C"
{
	// void exactinit();
	double orient2d(const double* pa, const double* pb, const double* pc);
	double orient3d(const double* pa, const double* pb, const double* pc, const double* pd);
	double orient3dfast(const double* pa, const double* pb, const double* pc, const double* pd);
	double incircle(const double* pa, const double* pb, const double* pc, const double* pd);
	double insphere(
		const double* pa, const double* pb, const double* pc, const double* pd, const double* pe);
}

#ifdef MCUT_WITH_ARBITRARY_PRECISION_NUMBERS

#include "nfg/numerics.h" // Indirect_Predicates

class rational_number : public bigrational
{
public:
	rational_number()
		: bigrational(bigfloat(0.0))
	{ }

	rational_number(double val)
		: bigrational(bigfloat(val))
	{ }

	rational_number(bigrational val)
		: bigrational(val)
	{ }

	virtual ~rational_number() { }

    inline static rational_number zero() 
	{
		return rational_number(0.0);
	}

    inline static rational_number one() 
	{
		return rational_number(1.0);
	}

	explicit operator double() const
	{
		return this->get_d();
	}

    //
    // some operators that are not defined in "bigrational" class
    //

	rational_number& operator-=(const rational_number& rhs)
	{
		auto x = *this - rhs;
		*this = x;
		return *this;
	}

	rational_number& operator+=(const rational_number& rhs)
	{
		auto x = *this + rhs;
		*this = x;
		return *this;
	}

	rational_number& operator*=(const rational_number& rhs)
	{
		auto x = *this * rhs;
		*this = x;
		return *this;
	}

	rational_number& operator/=(const rational_number& rhs)
	{
		auto x = *this / rhs;
		*this = x;
		return *this;
	}

	bool operator<(const rational_number& r) const
	{
		return bigrational::operator<(r);
	}

    //
    // static member functions
    //

	static rational_number abs(rational_number _a)
	{
		if(_a < rational_number::zero())
		{
			auto copy = _a;
			copy.negate();
			return copy;
		}

		return _a;
	}

	static rational_number min(rational_number _a, rational_number _b)
	{
		return _a <= _b ? _a : _b;
	}

	static rational_number max(rational_number _a, rational_number _b)
	{
		return _a > _b ? _a : _b;
	}

	static rational_number clamp(rational_number n, rational_number lower, rational_number upper)
	{
		return rational_number::max(lower, rational_number::min(n, upper));
	}

    static rational_number quantize(const double& d /*double prec value*/,
									   const double& m /*multiplier*/)
	{
		MCUT_ASSERT(std::abs(d)<=m);
		MCUT_ASSERT(m != 0);

		if(d == 0)
		{
			return rational_number::zero();
        }
		
        //  map all into normalized range [-1, 1]^3.
		const auto n = d / m;
		// remap into integer range [-2^26, 2^26]^3.
		const auto i = int(n * (1 << 26));
		const rational_number result(i);
		return result;
	}

	static double dequantize(const rational_number& i /*rational*/,
								 const double& m /*multiplier*/)
	{
		if(i == rational_number::zero())
		{
			return (0.0);
		}
		//std::cout << i << std::endl;
		// from integer range [-2^26, 2^26]^3 to normalized range [-1, 1]^3
		MCUT_ASSERT(i <= rational_number(1 << 26));
		const auto n = i / rational_number(1 << 26);
		//std::cout << n.get_dec_str() << std::endl;
		//auto nv = n.get_d();
		// from normalized range [-1, 1]^3 to actual/user coord value
		const auto d = n * rational_number(m);
		const double result = d.get_d(); // NOTE: truncated

        MCUT_ASSERT(result <= m);

		return result;
	}
};

typedef rational_number scalar_t;
#else
typedef double scalar_t;



#endif // MCUT_WITH_ARBITRARY_PRECISION_NUMBERS

enum sign_t {
    ON_NEGATIVE_SIDE = -1, // left
    ON_ORIENTED_BOUNDARY = 0, // on boundary
    ON_POSITIVE_SIDE = 1, // right
    //
    NEGATIVE = ON_NEGATIVE_SIDE,
    ZERO = ON_ORIENTED_BOUNDARY,
    POSITIVE = ON_POSITIVE_SIDE,
};

template <typename T = scalar_t>
class vec2_ {
public:
    typedef T element_type;
    vec2_()
        : m_x(0.0)
        , m_y(0.0)
    {
    }

    vec2_(const T& value)
        : m_x(value)
        , m_y(value)
    {
    }

    vec2_(const T& x, const T& y)
        : m_x(x)
        , m_y(y)
    {
    }

    virtual ~vec2_()
    {
    }

    static vec2_ make(const T x, const T y)
    {
        return vec2_<T>(x, y);
    }

    static int cardinality()
    {
        return 2;
    }

    bool operator==(const vec2_<T>& other) const
    {
        return this->m_x == other.x() && this->m_y == other.y();
    }

    const T& operator[](int index) const
    {
        MCUT_ASSERT(index >= 0 && index <= 1);

        if (index == 0) {
            return m_x;
        } else {
            return m_y;
        }
    }

    T& operator[](int index)
    {
        MCUT_ASSERT(index >= 0 && index <= 1);

        if (index == 0) {
            return m_x;
        } 

            return m_y;
        
    }

    const vec2_ operator-(const vec2_& other) const
    {
        return vec2_(m_x - other.m_x, m_y - other.m_y);
    }

    const vec2_ operator+(const vec2_& other) const
    {
        return vec2_(m_x + other.m_x, m_y + other.m_y);
    }

    const vec2_ operator/(const T& number) const
    {
        return vec2_(m_x / number, m_y / number);
    }

    const vec2_ operator*(const T& number) const
    {
        return vec2_(m_x * number, m_y * number);
    }

    const T& x() const
    {
        return m_x;
    }

    const T& y() const
    {
        return m_y;
    }

     T& x()  
    {
        return m_x;
    }

      T& y()  
    {
        return m_y;
    }

protected:
    T m_x, m_y;
}; // vec2_

typedef vec2_<> vec2;

template <typename T = scalar_t>
class vec3_ : public vec2_<T> {
public:
    vec3_()
        : vec2_<T>(0.0, 0.0)
        , m_z(0.0)
    {
    }

    vec3_(const T& value)
        : vec2_<T>(value, value)
        , m_z(value)
    {
    }

    vec3_(const T& x, const T& y, const T& z)
        : vec2_<T>(x, y)
        , m_z(z)
    {
    }
    ~vec3_()
    {
    }

    static int cardinality()
    {
        return 3;
    }

    const T& operator[](int index) const
    {
        MCUT_ASSERT(index >= 0 && index <= 2);
        if (index == 0) {
            return this->m_x;
        } else if (index == 1) {
            return this->m_y;
        } else {
            return this->m_z;
        }
    }

    T& operator[](int index)
    {
        MCUT_ASSERT(index >= 0 && index <= 2);
        if (index == 0) {
            return this->m_x;
        } else if (index == 1) {
            return this->m_y;
        } else {
            return this->m_z;
        }
    }

    vec3_ operator-(const vec3_& other) const
    {
        return vec3_(this->m_x - other.m_x, this->m_y - other.m_y, this->m_z - other.m_z);
    }

    vec3_ operator+(const vec3_& other) const
    {
        return vec3_(this->m_x + other.m_x, this->m_y + other.m_y, this->m_z + other.m_z);
    }

    const vec3_ operator/(const T& number) const
    {
        return vec3_(this->m_x / number, this->m_y / number, this->m_z / number);
    }

    const vec3_ operator*(const T& number) const
    {
        return vec3_(this->m_x * number, this->m_y * number, this->m_z * number);
    }

    const T& z() const
    {
        return m_z;
    }

    T& z() 	{
		return m_z;
	}

protected:
    T m_z;
}; // vec3_

typedef vec3_<> vec3;

template <typename T = scalar_t>
class matrix_t {
public:
    matrix_t()
        : m_rows(-1)
        , m_cols(-1)
    {
    }

    matrix_t(unsigned int rows, unsigned int cols)
        : m_rows(rows)
        , m_cols(cols)
        , m_entries(std::vector<T>((size_t)rows * cols, T(0.0))) // zeroes
    {
    }

    T& operator()(unsigned int row, unsigned int col)
    {
        return m_entries[(size_t)row * m_cols + col];
    }

    T operator()(unsigned int row, unsigned int col) const
    {
        return m_entries[(size_t)row * m_cols + col];
    }

    // multiply
    matrix_t<T> operator*(const matrix_t<T>& other) const
    {
        matrix_t<T> result(this->rows(), other.cols());

        for (int i = 0; i < this->rows(); ++i) {
            for (int j = 0; j < other.cols(); ++j) {
                for (int k = 0; k < this->cols(); ++k) {
                    result(i, j) += (*this)(i, k) * other(k, j);
                }
            }
        }

        return result;
    }

    matrix_t<T> operator*(const scalar_t& s) const
    {
        matrix_t<T> result(m_rows, m_cols);

        for (int i = 0; i < m_rows; ++i) {
            for (int j = 0; j < m_cols; ++j) {
                result(i, j) = (*this)(i, j) * s;
            }
        }

        return result;
    }

    matrix_t<T> operator/(const scalar_t& s) const
    {
        matrix_t<T> result(m_rows, m_cols);

        for (int i = 0; i < m_rows; ++i) {
            for (int j = 0; j < m_cols; ++j) {
                result(i, j) = (*this)(i, j) / s;
            }
        }

        return result;
    }

    matrix_t<T> operator-(const matrix_t<T>& m) const
    {
        MCUT_ASSERT(m.rows() == this->rows());
        MCUT_ASSERT(m.cols() == this->cols());

        matrix_t<T> result(m_rows, m_cols);

        for (int i = 0; i < m_rows; ++i) {
            for (int j = 0; j < m_cols; ++j) {
                result(i, j) = (*this)(i, j) - m(i, j);
            }
        }

        return result;
    }

    // 2x3 matrix times 3x1 vector
    vec2 operator*(const vec3& v) const
    {
        MCUT_ASSERT(this->cols() == vec3::cardinality());
        vec2 result(scalar_t(0.0));
        MCUT_ASSERT(this->rows() == vec2::cardinality());

        for (int col = 0; col < this->cols(); ++col) {
            for (int row = 0; row < vec2::cardinality(); ++row) {
                result[row] = result[row] + ((*this)(row, col) * v[col]);
            }
        }
#if 0
        // columns
        const Vec c0((*this)(0, 0), (*this)(1, 0), (*this)(2, 0));
        const Vec c1((*this)(0, 1), (*this)(1, 1), (*this)(2, 1));

        result = c0 * v[0] + c1 * v[1];

        if (this->rows() == 3 && (this->cols() == 3)
        {
            const Vec c2((*this)(0, 2), (*this)(1, 2), (*this)(2, 2));
            result = result + c2 * v[2];
        }
#endif
        return result;
    }

    inline int rows() const
    {
        return m_rows;
    }

    inline int cols() const
    {
        return m_cols;
    }

private:
    int m_rows;
    int m_cols;
    std::vector<T> m_entries;
};

template <typename T >
T square_root(const T& number, double multiplier = 1)
{
#if !defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)
		return std::sqrt(number);
#else
	const double dequantized = scalar_t::dequantize(
		number, multiplier); // to native user coordinates/from rational coordinates
	
	const double sqrt_val = std::sqrt(dequantized);
	const scalar_t quantized =
		scalar_t::quantize(sqrt_val, multiplier); // to rational coordinates
	return quantized;
#endif // #if !defined(MCUT_WITH_ARBITRARY_PRECISION_NUMBERS)
}

template <>
inline double square_root(const double& x, double)
{
	return std::sqrt(x);
}

extern scalar_t absolute_value(const scalar_t& number);
extern sign_t sign(const scalar_t& number);
extern std::ostream& operator<<(std::ostream& os, const vec3& v);

template <typename U>
std::ostream& operator<<(std::ostream& os, const matrix_t<U>& m)
{
    for (int i = 0; i < m.rows(); i++) {
        for (int j = 0; j < m.cols(); j++) {
            os << m(i, j) << ", ";
        }
        os << "\n";
    }
    return os;
}

template <typename T>
const T& min(const T& a, const T& b)
{
    return ((b < a) ? b : a);
}
template <typename T>
const T& max(const T& a, const T& b)
{
    return ((a < b) ? b : a);
}

extern bool operator==(const vec3& a, const vec3& b);

template <typename T>
vec2_<T> compwise_min(const vec2_<T>& a, const vec2_<T>& b)
{
    return vec2_<T>(min(a.x(), b.x()), min(a.y(), b.y()));
}

template <typename T>
vec2_<T> compwise_max(const vec2_<T>& a, const vec2_<T>& b)
{
    return vec2_<T>(max(a.x(), b.x()), max(a.y(), b.y()));
}

template <typename T>
vec3_<T> compwise_min(const vec3_<T>& a, const vec3_<T>& b)
{
    return vec3_<T>(min(a.x(), b.x()), min(a.y(), b.y()), min(a.z(), b.z()));
}

template <typename T>
vec3_<T> compwise_max(const vec3_<T>& a, const vec3_<T>& b)
{
    return vec3_<T>(max(a.x(), b.x()), max(a.y(), b.y()), max(a.z(), b.z()));
}

template <typename vector_type>
vector_type cross_product(const vector_type& a, const vector_type& b)
{ 
    return vector_type( a.y() * b.z() - a.z() * b.y(),
				        a.z() * b.x() - a.x() * b.z(),
				        a.x() * b.y() - a.y() * b.x());
}

template <typename vector_type>
typename vector_type::element_type dot_product(const vector_type& a, const vector_type& b)
{
	typename vector_type::element_type out(0.0);
    for (int i = 0; i < vector_type::cardinality(); ++i) {
        out += (a[i] * b[i]);
    }
    return out;
}

// compute a*b^T, which is a matrix
template <typename vector_type>
matrix_t<typename vector_type::element_type> outer_product(const vector_type& a, const vector_type& b)
{
    matrix_t<typename vector_type::element_type> out(vector_type::cardinality(), vector_type::cardinality());
    const vector_type c0 = a * b[0]; // colmuns
    const vector_type c1 = a * b[1];

    if (vector_type::cardinality() == 3) {
        const vector_type c2 = a * b[2];

        out(0, 0) = c0[0];
        out(1, 0) = c0[1];
        out(2, 0) = c0[2];

        out(0, 1) = c1[0];
        out(1, 1) = c1[1];
        out(2, 1) = c1[2];

        out(0, 2) = c2[0];
        out(1, 2) = c2[1];
        out(2, 2) = c2[2];
    } else {
        out(0, 0) = c0[0];
        out(1, 0) = c0[1];

        out(0, 1) = c1[0];
        out(1, 1) = c1[1];
    }

    return out;
}

template <typename vector_type>
typename vector_type::element_type squared_length(const vector_type& v)
{
    return dot_product(v, v);
}
#	if MCUT_WITH_ARBITRARY_PRECISION_NUMBERS
template <typename vector_type>
typename vector_type::element_type length(const vector_type& v, double multiplier = 1.0)
{
	MCUT_ASSERT(false);
    typename vector_type::element_type foo;
	return foo; // no-op
}
template <>
inline scalar_t length(const vec3_<scalar_t>& v, double /*multiplier*/)
{
	return std::sqrt(squared_length(v).get_d());
	//square_root(squared_length(v).get_d(), multiplier);
}

template <>
inline double length(const vec2_<double>& v, double multiplier)
{
	return square_root(squared_length(v), multiplier);
}

template <>
inline double length(const vec3_<double>& v, double multiplier)
{
	return square_root(squared_length(v), multiplier);
}

#else
template <typename vector_type>
typename vector_type::element_type length(const vector_type& v, double multiplier = 1.0)
{
	return square_root(squared_length(v), multiplier);
}
#endif

template <typename vector_type>
vector_type normalize(const vector_type& v, double multiplier = 1.0)
{
	return v / length(v, multiplier);
}

scalar_t orient2d(const vec2& pa, const vec2& pb, const vec2& pc);
scalar_t orient3d(const vec3& pa, const vec3& pb, const vec3& pc,
    const vec3& pd);

// Compute a polygon's plane coefficients (i.e. normal and d parameters).
// The computed normal is not normalized. This function returns the largest component of the normal.
int compute_polygon_plane_coefficients(vec3& normal, scalar_t& d_coeff,
									   const vec3* polygon_vertices,
									   const int polygon_vertex_count,
									   const double multiplier);

// Compute the intersection point between a line (not a segment) and a plane defined by a polygon.
//
// Parameters:
//  'p' : output intersection point (computed if line does indeed intersect the plane)
//  'q' : first point defining your line
//  'r' : second point defining your line
//  'polygon_vertices' : the vertices of the polygon defineing the plane (assumed to not be degenerate)
//  'polygon_vertex_count' : number of olygon vertices
//  'polygon_normal_max_comp' : largest component of polygon normal.
//  'polygon_plane_normal' : normal of the given polygon
//  'polygon_plane_d_coeff' : the distance coefficient of the plane equation corresponding to the polygon's plane
//
// Return values:
// '0': line is parallel to plane or polygon is degenerate (within available precision)
// '1': an intersection exists.
// 'p': q and r lie in the plane (technically they are parallel to the plane too).
char compute_line_plane_intersection(vec3& p, // intersection point
    const vec3& q, const vec3& r, const vec3* polygon_vertices,
    const int polygon_vertex_count, const int polygon_normal_max_comp,
    const vec3& polygon_plane_normal);

// Test if a line segment intersects with a plane, and yeild the intersection point if so.
//
// Return values:
// 'p': The segment lies wholly within the plane.
// 'q': The(first) q endpoint is on the plane (but not 'p').
// 'r' : The(second) r endpoint is on the plane (but not 'p').
// '0' : The segment lies strictly to one side or the other of the plane.
// '1': The segment intersects the plane, and none of {p, q, r} hold.
char compute_segment_plane_intersection(vec3& p, const vec3& normal, const scalar_t& d_coeff,
    const vec3& q, const vec3& r);

// Similar to "compute_segment_plane_intersection" but simply checks the [type] of intersection using
// exact arithmetic
//
// Return values:
// 'p': The segment lies wholly within the plane.
// 'q': The(first) q endpoint is on the plane (but not 'p').
// 'r' : The(second) r endpoint is on the plane (but not 'p').
// '0' : The segment lies strictly to one side or the other of the plane.
// '1': The segment intersects the plane, and none of {p, q, r} hold.
char compute_segment_plane_intersection_type(const vec3& q, const vec3& r,
    const std::vector<vec3>& polygon_vertices,
											 const vec3& polygon_normal,
											 const int polygon_normal_largest_component,
											 const double multiplier);

// Test if a point 'q' (in 2D) lies inside or outside a given polygon (count the number ray crossings).
//
// Return values:
// 'i': q is strictly interior
// 'o': q is strictly exterior (outside).
// 'e': q is on an edge, but not an endpoint.
// 'v': q is a vertex.
char compute_point_in_polygon_test(const vec2& q,
								   const std::vector<vec2>& polygon_vertices);

// Test if a point 'q' (in 3D) lies inside or outside a given polygon (count the number ray crossings).
//
// Return values:
// 'i': q is strictly interior
// 'o': q is strictly exterior (outside).
// 'e': q is on an edge, but not an endpoint.
// 'v': q is a vertex.
char compute_point_in_polygon_test(const vec3& p, const std::vector<vec3>& polygon_vertices,
    const vec3& polygon_normal, const int polygon_normal_largest_component, const double multiplier);

// project a 3d polygon to 3d by eliminating the largest component of its normal
void project_to_2d(std::vector<vec2>& out, const std::vector<vec3>& polygon_vertices,
				   const vec3& polygon_normal,
				   const int polygon_normal_largest_component,
				   const double multiplier);

//void project_to_2d(std::vector<vec2>& out, const std::vector<vec3>& polygon_vertices,
//				   const vec3& polygon_normal,
//				   const double multiplier);

bool coplaner(const vec3& pa, const vec3& pb, const vec3& pc,
    const vec3& pd);

#	ifdef MCUT_WITH_ARBITRARY_PRECISION_NUMBERS
static bool collinear(const vec2_<double>& a,
			   const vec2_<double>& b,
			   const vec2_<double>& c,
			   double& predResult)
{
#ifdef MCUT_WITH_ARBITRARY_PRECISION_NUMBERS
	const double pa_[2] = {a.x(), a.y()};
	const double pb_[2] = {b.x(), b.y()};
	const double pc_[2] = {c.x(), c.y()};

	predResult = ::orient2d(pa_, pb_, pc_); // shewchuk predicate
    #else
	predResult = orient2d(a, b, c);
    #endif

	return predResult == double(0.);
}
#endif


bool collinear(const vec2& a, const vec2& b, const vec2& c, scalar_t& predResult);

bool collinear(const vec2& a, const vec2& b, const vec2& c);


char Parallellntd(const vec2_<double>& a,
				 const vec2_<double>& b,
				 const vec2_<double>& c,
				 const vec2_<double>& d,
				 vec2_<double>& p);

/*
Compute the intersection of two line segments. Can also be used to calculate where the respective lines intersect.

Parameters:
'a' and 'b': end points of first segment
'c' and 'd': end points of second segment
'p': the intersection point
's': the parameter for parametric equation of segment a,b (0..1)
't': the parameter for parametric equation of segment c,d (0..1)

Return values:

'e': The segments collinearly overlap, sharing a point; 'e' stands for 'edge.'
'v': An endpoint of one segment is on the other segment, but 'e' doesn't hold; 'v' stands for 'vertex.'
'1': The segments intersect properly (i.e., they share a point and neither 'v' nor 'e' holds); '1' stands for TRUE.
'0': The segments do not intersect (i.e., they share no points); '0' stands for FALSE
*/
char compute_segment_intersection(const vec2& a, const vec2& b, const vec2& c, const vec2& d,
    vec2& p, scalar_t& s, scalar_t& t);

template <typename vector_type>
struct bounding_box_t {

    vector_type m_minimum;
    vector_type m_maximum;

    bounding_box_t(const vector_type& minimum, const vector_type& maximum)
    {
        m_minimum = minimum;
        m_maximum = maximum;
    }

    bounding_box_t()
    {
        m_minimum = vector_type(std::numeric_limits<double>::max());
        m_maximum = vector_type(-std::numeric_limits<double>::max());
    }

    inline const vector_type& minimum() const
    {
        return m_minimum;
    }

    inline const vector_type& maximum() const
    {
        return m_maximum;
    }

    inline void expand(const vector_type& point)
    {
        m_maximum = compwise_max(m_maximum, point);
        m_minimum = compwise_min(m_minimum, point);
    }

    inline void expand(const bounding_box_t<vector_type>& bbox)
    {
        m_maximum = compwise_max(m_maximum, bbox.maximum());
        m_minimum = compwise_min(m_minimum, bbox.minimum());
    }

    inline void enlarge(const typename vector_type::element_type& eps_)
    {
        m_maximum = m_maximum + eps_;
        m_minimum = m_minimum - eps_;
    }

    float SurfaceArea() const
    {
        vector_type d = m_maximum - m_minimum;
        return typename vector_type::element_type(2.0) * (d.x() * d.y() + d.x() * d.z() + d.y() * d.z());
    }

    int MaximumExtent() const
    {
        vector_type diag = m_maximum - m_minimum;
        if (diag.x() > diag.y() && diag.x() > diag.z())
            return 0;
        else if (diag.y() > diag.z())
            return 1;
        else
            return 2;
    }
};


template <typename T>
inline bool intersect_bounding_boxes(const bounding_box_t<vec3_<T>>& a, const bounding_box_t<vec3_<T>>& b)
{
    const vec3_<T>& amin = a.minimum();
    const vec3_<T>& amax = a.maximum();
    const vec3_<T>& bmin = b.minimum();
    const vec3_<T>& bmax = b.maximum();
    return (amin.x() <= bmax.x() && amax.x() >= bmin.x()) && //
        (amin.y() <= bmax.y() && amax.y() >= bmin.y()) && //
        (amin.z() <= bmax.z() && amax.z() >= bmin.z());
}

bool point_in_bounding_box(const vec2& point, const bounding_box_t<vec2>& bbox);

bool point_in_bounding_box(const vec3& point, const bounding_box_t<vec3>& bbox);

template <typename vector_type>
void make_bbox(bounding_box_t<vector_type>& bbox, const vector_type* vertices, const int num_vertices)
{
    MCUT_ASSERT(vertices != nullptr);
    MCUT_ASSERT(num_vertices >= 3);

    for (int i = 0; i < num_vertices; ++i) {
        const vector_type& vertex = vertices[i];
        bbox.expand(vertex);
    }
}


#ifdef MCUT_WITH_ARBITRARY_PRECISION_NUMBERS
static scalar_t orient2d(const scalar_t* pa, const scalar_t* pb, const scalar_t* pc)
{
	auto acx = pa[0] - pc[0];
	auto bcx = pb[0] - pc[0];
	auto acy = pa[1] - pc[1];
	auto bcy = pb[1] - pc[1];

	return acx * bcy - acy * bcx;
}
static scalar_t
orient3d(const scalar_t* pa, const scalar_t* pb, const scalar_t* pc, const scalar_t* pd)
{
	auto adx = pa[0] - pd[0];
	auto bdx = pb[0] - pd[0];
	auto cdx = pc[0] - pd[0];
	auto ady = pa[1] - pd[1];
	auto bdy = pb[1] - pd[1];
	auto cdy = pc[1] - pd[1];
	auto adz = pa[2] - pd[2];
	auto bdz = pb[2] - pd[2];
	auto cdz = pc[2] - pd[2];

	return adx * (bdy * cdz - bdz * cdy) + bdx * (cdy * adz - cdz * ady) +
		   cdx * (ady * bdz - adz * bdy);
}

static double orient2d(const vec2_<double>& pa, const vec2_<double>& pb, const vec2_<double>& pc)
{
	const double pa_[2] = {pa.x(), pa.y()};
	const double pb_[2] = {pb.x(), pb.y()};
	const double pc_[2] = {pc.x(), pc.y()};

	return ::orient2d(pa_, pb_, pc_); // shewchuk predicate
}
#endif



#endif // MCUT_MATH_H_
