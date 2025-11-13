/* Copyright EPFL 2023
 * Joshua Klein
 *
 * Layer definitions for custom single-core LeNet5 CNN based on LeNet5 with
 * MNIST provided by Flavio Ponzina.
 *
 */

#ifndef __LENET5_HH__
#define __LENET5_HH__

// Number of inferences.
const int T_x = 1;

// Layers.
conv_layer_args conv1 = conv_layer_args(
    0,                  // Layer number.
    T_x,                // Number of inferences.
    
    true,               // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    32,                 // Input height.
    32,                 // Input width.
    1,                  // Input channels.
    5,                  // Kernel height.
    5,                  // Kernel width.
    6,                  // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

pool_layer_args pool1 = pool_layer_args(
    1,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv1.output_h,     // Input height.
    conv1.output_w,     // Input width.
    conv1.output_c,     // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
    2,                  // Pool factor.
    1,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv2 = conv_layer_args(
    2,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool1.output_h,     // Input height.
    pool1.output_w,     // Input width.
    pool1.output_c,     // Input channels.
    5,                  // Kernel height.
    5,                  // Kernel width.
    16,                 // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

pool_layer_args pool2 = pool_layer_args(
    3,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv2.output_h,     // Input height.
    conv2.output_w,     // Input width.
    conv2.output_c,     // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
    2,                  // Pool factor.
    1,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv3 = conv_layer_args(
    4,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool2.output_h,     // Input height.
    pool2.output_w,     // Input width.
    pool2.output_c,     // Input channels.
    5,                  // Kernel height.
    5,                  // Kernel width.
    120,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

flatten_layer_args flatten1 = flatten_layer_args(
    5,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv3.output_h,     // Input height.
    conv3.output_w,     // Input width.
    conv3.output_c,     // Input channels.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

fc_layer_args dense1 = fc_layer_args(
    6,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    flatten1.output_size,   // Input size.

    120,                // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

fc_layer_args dense2 = fc_layer_args(
    7,                  // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    true,               // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    dense1.output_size, // Input size.

    84,                 // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

#endif // __LENET5_HH__
