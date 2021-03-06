#ifndef WAVENET_UTILITIES_H
#define WAVENET_UTILITIES_H

/**
 * @file   Utilities.h
 * @author Andreas Sogaard
 * @date   15 November 2016
 * @brief  Collection of utility functions.
 */

// STL include(s).
#include <cmath> /* log2, log10, sqrt */
#include <algorithm> /* std::max */
#include <cstdio> /* snprintf */
#include <sys/stat.h> /* struct stat */
#include <cassert> /* assert */
#include <memory> /* std::unique_ptr */
#include <utility> /* std::move */
#include <string> /* std::string */
#include <sstream> /* std::stringstream */

#ifdef USE_ROOT
// ROOT include(s).
#include "TH2.h"
#include "TGraph.h"
#endif // USE_ROOT

// Armadillo include(s).
#include <armadillo>

// Wavenet include(s).
#include "Wavenet/Logger.h"


namespace wavenet {

class Wavenet; /* To resolve circular dependence. */

/// Global constants.
const double EPS = 1.0e-12;
const double PI  = 3.14159265359;


/// Math functions.
/**
 * Determine wether the given number is radix 2, i.e. satisfies: 
 *   y = 2^{x} \in \mathbb{N}
 */
inline bool isRadix2(const unsigned& x) { 
    double l2 = log2(x);
    return (l2 - int(l2)) == 0;
}

/**
 * Square of number.
 */
template<class T>
inline T sq(const T& x) { return x * x; }

/**
 * Absolute value of number.
 */
template<class T>
inline T norm(const T& x) { return sqrt(sq(x)); }

/**
 * Sign of number.
 */
template<class T>
inline T sign(const T& x) { return x == 0 ? 0 : x/norm(x); }

/**
 * Sign of all numbers in Armadillo column vector-type container.
 */
template<class T> 
inline arma::Col<T> sign(const arma::Col<T>& v) { 
    arma::Col<T> s = v; 
    return s.transform( [](double x) { return sign(x); } ); 
}

/**
 * Sign of all numbers in Armadillo matrix-type container.
 */
template<class T> 
inline arma::Mat<T> sign(const arma::Mat<T>& M) { return M / abs(M + EPS);  }


/// Path functions.
/**
 * Check whether file exists.
 */
inline bool fileExists (const std::string& filename) {
    ifstream f(filename.c_str());
    bool exists = f.good();
    f.close();
    return exists;
}

/**
 * Check whether directory exists.
 */
inline bool dirExists (const std::string& dir) {
    struct stat statbuf;
    bool exists = false;
    if (stat(dir.c_str(), &statbuf) != -1) {
        if (S_ISDIR(statbuf.st_mode)) { exists = true; }
    }
    return exists;
}


/// String functions.
/**
 * Check whether string contains only numeric characters.
 */
inline bool isNumber (const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !(std::isdigit(c) || strcmp(&c, ".") == 0 || (strcmp(&c, " ") == 0)); }) == s.end();
}

/**
 * Check whether string contains only whitespaces.
 */
inline bool isEmpty (const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !(strcmp(&c, " ") == 0); }) == s.end();
}

/**
 * Split a delimeter-separated string into elements.
 * 
 * Taken from [http://stackoverflow.com/a/236803]
 */
inline std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/**
 * Format a floating point number to a string.
 */
inline std::string formatNumber (const double&   f,
                                 const unsigned& precision = 2,
                                 const bool& significantDigits = false,
                                 const unsigned& leadingPlaces = 0) {
    const unsigned l = 100;
    char buffer [l];
    int cx = snprintf ( buffer, l, "%*.*f",
        leadingPlaces,
        (f == 0 ? 0 : (significantDigits ? std::max(int(precision) - 1 - int(log10(f/2. + EPS)), 0) : precision)), \
        f );
    std::string s = buffer;
    return s;
}


/// Armadillo-specific functions.
/**
 * Generate uniformly random point on N-sphere.
 */
arma::Col<double> PointOnNSphere (const unsigned& N, const double& rho = 0.);

/**
 * Given a collection of 1D neural network activations, return the corresponding 
 * vector of wavelet coefficients.
 */
arma::Col<double> coeffsFromActivations (const arma::field< arma::Col<double> >& activations);
    
/**
 * Given a collection of 2D neural network activations, return the corresponding 
 * matrix of wavelet coefficients.
 */
arma::Mat<double> coeffsFromActivations (const std::vector< std::vector< arma::field< arma::Col<double> > > >& Activations);
    

/// ROOT-specific functions.
#ifdef USE_ROOT

/**
 * Return ROOT TGraph of a cost log. By default, the internal cost log of the 
 * wavenet instance is used.
 */
TGraph costGraph (const std::vector< double >& costLog);

/**
 * Convert arma matrix to ROOT TH2 histogram.
 */
std::unique_ptr<TH1> MatrixToHist2D (const arma::Mat<double>& matrix, const double& range);

/**
 * Convert arma matrix to ROOT TH1 histogram.
 */
std::unique_ptr<TH1> MatrixToHist1D (const arma::Mat<double>& matrix, const double& range);

/**
 * Convert arma matrix to ROOT histogram, with type dynamically determined.
 */
std::unique_ptr<TH1> MatrixToHist   (const arma::Mat<double>& matrix, const double& range);

/**
 * Fill the provided Armadillo matrix with the contents of a ROOT TH2 histogram.
 */
arma::Mat<double> HistFillMatrix2D (const TH1* hist, arma::Mat<double>& matrix);

/**
 * Fill the provided Armadillo matrix with the contents of a ROOT TH1 histogram.
 */
arma::Mat<double> HistFillMatrix1D (const TH1* hist, arma::Mat<double>& matrix);

/**
 * Fill the provided Armadillo matrix with the contents of a ROOT histogram, 
 * with type dynamically determined.
 */
arma::Mat<double> HistFillMatrix   (const TH1* hist, arma::Mat<double>& matrix);

/**
 * Convert ROOT TH2 histogram to an Armadillo matrix (2D).
 */
arma::Mat<double> HistToMatrix2D (const TH1* hist);

/**
 * Convert ROOT TH1 histogram to an Armadillo matrix (1D).
 */
arma::Mat<double> HistToMatrix1D (const TH1* hist);

/**
 * Convert ROOT histogram, with type dynamically determined, to an Armadillo 
 * matrix.
 */
arma::Mat<double> HistToMatrix   (const TH1* hist);

#endif // USE_ROOT

} // namespace

#endif // WAVENET_UTILITIES_H
