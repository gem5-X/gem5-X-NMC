/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Copyright EPFL 2025
 * Riselda Kodra
 * 
 * This file is for organizing the includes necessary for running the Hybrid
 * CNNs.  Used primarily to keep main code slightly cleaner.
 *
 * Extended with NMC
 */

#ifndef __TINYTENSORLIB_HH__
#define __TINYTENSORLIB_HH__

// Eigen library stuff.
#include <Eigen/Core>
#include <Eigen/Dense>
#include <unsupported/Eigen/CXX11/Tensor>
using namespace Eigen;
#ifndef CNM 
typedef Tensor<int8_t, 1> TB_Vector;
typedef Tensor<int8_t, 2> TB_Matrix2D;
typedef Tensor<int8_t, 3> TB_Matrix3D;
#else
typedef Tensor<Eigen::half, 1> TB_Vector;
typedef Tensor<Eigen::half, 2> TB_Matrix2D;
typedef Tensor<Eigen::half, 3> TB_Matrix3D;
#endif

// Main layer, function, utilty definitions.
#include "layer.hh"
#include "functions.hh"

#include "utilities.hh"

#endif // __TINYTENSORLIB_HH__
