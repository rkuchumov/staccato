#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <cstdlib>
#include <cmath>
#include <cfloat>

class Counter
{
public:
	Counter (size_t cutoff = 0) {
		n = 0;
		m1 = m2 = m3 = m4 = 0;
		m_max = -FLT_MAX;
		m_min = FLT_MAX;
		m_cutoff = cutoff;
	}

	Counter &operator +=(const double &x) {
		if (m_cutoff > 0) {
			m_cutoff--;
			return *this;
		}

		n++;

        double d_n = (x - m1) / n;
        double t = (x - m1) * d_n * (n - 1);

        m4 += d_n*d_n * (t * (n*n - 3.f*n + 3.f) + 6.f*m2) - 4.f*d_n*m3;
        m3 += t * d_n * (n - 2.f) - 3.f * d_n * m2;
        m2 += t;
        m1 += d_n;

		if (x > m_max) m_max = x;
		if (x < m_min) m_min = x;
		return *this;
	}

	double min() const {
		return m_min;
	}

	double max() const {
		return m_max;
	}

	size_t count() const {
		return n;
	}

	double mean() const {
		return m1;
	}

	double var() const {
		return m2 / (n - 1);
	}

	double sd() const {
		return sqrt(var());
	}

	double se() const {
		return sqrt(var() / n);
	}

	double pm(double qt) const {
		return qt * se() * 100 / mean();
	}

	double skewness() const {
		return (sqrt(n) * m3) / sqrt(m2 * m2 * m2) ;
	}

	double kurtosis() const {
		return (n * m4) / (m2 * m2);
	}

private:
	double n;
	double m1, m2, m3, m4;
	double m_min;
	double m_max;
	size_t m_cutoff;
};

#endif /* end of include guard: COUNTER_HPP */

