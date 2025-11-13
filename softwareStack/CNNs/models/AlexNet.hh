/* Copyright EPFL 2023
 * Joshua Klein
 *
 * Layer definitions for custom single-core AlexNet CNN.
 */

#ifndef __ALEXNET_HH__
#define __ALEXNET_HH__

// Number of inferences.
const int T_x = 1;
int layer_num = 0;

// Layers.
conv_layer_args conv1 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    true,               // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    227,                // Input height.
    227,                // Input width.
    3,                  // Input channels.
    11,                 // Kernel height.
    11,                 // Kernel width.
    96,                 // Number of filters.
    4,                  // Stride.

    0,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool1 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv1.output_h,     // Input height.
    conv1.output_w,     // Input width.
    conv1.output_c,     // Input channels.
    3,                  // Pool height.
    3,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args conv2 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool1.output_h,     // Input height.
    pool1.output_w,     // Input width.
    pool1.output_c,     // Input channels.
    5,                  // Kernel height.
    5,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    2,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool2 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv2.output_h,     // Input height.
    conv2.output_w,     // Input width.
    conv2.output_c,     // Input channels.
    3,                  // Pool height.
    3,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args conv3 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool2.output_h,     // Input height.
    pool2.output_w,     // Input width.
    pool2.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    384,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv4 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv3.output_h,     // Input height.
    conv3.output_w,     // Input width.
    conv3.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    384,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv5 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv4.output_h,     // Input height.
    conv4.output_w,     // Input width.
    conv4.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool3 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv5.output_h,     // Input height.
    conv5.output_w,     // Input width.
    conv5.output_c,     // Input channels.
    3,                  // Pool height.
    3,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

flatten_layer_args flatten1 = flatten_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool3.output_h,     // Input height.
    pool3.output_w,     // Input width.
    pool3.output_c,     // Input channels.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

fc_layer_args dense1 = fc_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    flatten1.output_size,   // Input size.

    4096,               // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

fc_layer_args dense2 = fc_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    dense1.output_size, // Input size.

    4096,               // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

fc_layer_args dense3 = fc_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    true,               // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    dense2.output_size, // Input size.

    1000,               // Output size.
    NO_NORM_TYPE,       // Normalization.
    SOFTMAX_ACT_TYPE    // Activation function.
);

#endif // __ALEXNET_HH__
