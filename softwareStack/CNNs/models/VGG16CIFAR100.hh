/* Copyright EPFL 2023
 * Joshua Klein
 *
 * Layer definitions for custom single-core VGG16 CNN based on VGG16 with
 * CIFAR100 provided by Flavio Ponzina.
 *
 */

#ifndef __VGG16CIFAR100_HH__
#define __VGG16CIFAR100_HH__

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
    32,                 // Input height.
    32,                 // Input width.
    3,                  // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv2 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv1.output_h,     // Input height.
    conv1.output_w,     // Input width.
    conv1.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool1 = pool_layer_args(
    layer_num++,        // Layer number.
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
    pool1.output_h,     // Input height.
    pool1.output_w,     // Input width.
    pool1.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
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
    128,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool2 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv4.output_h,     // Input height.
    conv4.output_w,     // Input width.
    conv4.output_c,     // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args conv5 = conv_layer_args(
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
    256,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv6 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv5.output_h,     // Input height.
    conv5.output_w,     // Input width.
    conv5.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv7 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv6.output_h,     // Input height.
    conv6.output_w,     // Input width.
    conv6.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
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
    conv7.output_h,     // Input height.
    conv7.output_w,     // Input width.
    conv7.output_c,     // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args conv8 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool3.output_h,     // Input height.
    pool3.output_w,     // Input width.
    pool3.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv9 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv8.output_h,     // Input height.
    conv8.output_w,     // Input width.
    conv8.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv10 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv9.output_h,     // Input height.
    conv9.output_w,     // Input width.
    conv9.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool4 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv10.output_h,    // Input height.
    conv10.output_w,    // Input width.
    conv10.output_c,    // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
    2,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    NO_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args conv11 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool4.output_h,     // Input height.
    pool4.output_w,     // Input width.
    pool4.output_c,     // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv12 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv11.output_h,    // Input height.
    conv11.output_w,    // Input width.
    conv11.output_c,    // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args conv13 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv12.output_h,    // Input height.
    conv12.output_w,    // Input width.
    conv12.output_c,    // Input channels.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.
#if defined (AIMC)
    true,               // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1,                  // Padding.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool5 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    false,              // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv13.output_h,    // Input height.
    conv13.output_w,    // Input width.
    conv13.output_c,    // Input channels.
    2,                  // Pool height.
    2,                  // Pool width.
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
    pool5.output_h,     // Input height.
    pool5.output_w,     // Input width.
    pool5.output_c,     // Input channels.
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
#if defined (AIMC)
    true,              // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    1000,               // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

fc_layer_args dense2 = fc_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    true,               // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    dense1.output_size, // Input size.
#if defined (AIMC)
    true,              // Are we using AIMC tiles?
    -1,                 // Allocated tile height? (-1 = infinite)
    -1,                 // Allocated tile width? (-1 = infinite)
#endif
    100,                // Output size.
    NO_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

#endif // __VGG16CIFAR100_HH__
