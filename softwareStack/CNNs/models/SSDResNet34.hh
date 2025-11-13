/* Copyright EPFL 2023
 * Joshua Klein
 *
 * Layer definitions for single-core SSD-ResNet34 CNN based [1].
 *
 * [1] https://www.kaggle.com/datasets/pytorch/resnet34
 *
 */

#ifndef __SSDRESNET34_HH__
#define __SSDRESNET34_HH__

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
    224,                // Input height.
    224,                // Input width.
    3,                  // Input channels.
    7,                  // Kernel height.
    7,                  // Kernel width.
    64,                 // Number of filters.
    2,                  // Stride.

    3,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool1 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    conv1,              // Input layer.
    2,                  // Pool dimension.
    3,                  // Pool factor.
    2,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args res2a_branch1 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool1,              // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args res2a_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool1,              // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2a_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2a_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2a_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2a_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res2a_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2a_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2b_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2a_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2b_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2b_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2b_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2b_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res2b_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2b_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2c_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2b_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2c_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2c_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    64,                 // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res2c_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2c_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res2c_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2c_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3a_branch1 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2c_branch2c,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args res3a_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res2c_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    128,                // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3a_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3a_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3a_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3a_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res3a_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3a_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3b_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3a_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3b_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3b_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3b_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3b_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res3b_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3b_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3c_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3b_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3c_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3c_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3c_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3c_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res3c_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3c_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3d_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3b_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3d_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3d_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    128,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res3d_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3d_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res3d_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3d_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4a_branch1 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3d_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args res4a_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res3d_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4a_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4a_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4a_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4a_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4a_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4a_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4b_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4a_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4b_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4b_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4b_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4b_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4b_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4b_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4c_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4b_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4c_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4c_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4c_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4c_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4c_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4c_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4d_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4c_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4d_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4d_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4d_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4d_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4d_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4d_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4e_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4d_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4e_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4e_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4e_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4e_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4e_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4e_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4f_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4e_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4f_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4f_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    256,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res4f_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4f_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    1024,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res4f_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4f_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5a_branch1 = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4f_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    2048,               // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

conv_layer_args res5a_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res4f_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    2,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5a_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5a_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5a_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5a_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    2048,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res5a_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5a_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5b_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5a_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5b_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5b_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5b_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5b_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    2048,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res5b_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5b_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5c_branch2a = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5b_end,          // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5c_branch2b = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5c_branch2a,     // Input layer.
    3,                  // Kernel height.
    3,                  // Kernel width.
    512,                // Number of filters.
    1,                  // Stride.

    1,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

conv_layer_args res5c_branch2c = conv_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5c_branch2b,     // Input layer.
    1,                  // Kernel height.
    1,                  // Kernel width.
    2048,               // Number of filters.
    1,                  // Stride.

    0,                  // Padding.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

end_residual_layer_args res5c_end = end_residual_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5c_branch2c,     // Input layer.
    BATCH_NORM_TYPE,       // Normalization.
    RELU_ACT_TYPE       // Activation function.
);

pool_layer_args pool2 = pool_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    res5c_end,          // Input layer.
    7,                  // Pool dimension.
    7,                  // Pool factor.
    1,                  // Stride.
    MAX_POOL_TYPE,      // Pooling type.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

flatten_layer_args flatten1 = flatten_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    SINGLE_BUFFER_TYPE, // Buffer type.
    pool2               // Input layer.
);

fc_layer_args dense1 = fc_layer_args(
    layer_num++,        // Layer number.
    T_x,                // Number of inferences.
    
    false,              // Is first layer.
    true,               // Is last layer.
    SINGLE_BUFFER_TYPE, // Buffer type.
    flatten1.output_size, // Input size.

    1000,               // Output size.
    BATCH_NORM_TYPE,       // Normalization.
    NO_ACT_TYPE         // Activation function.
);

#endif // __SSDRESNET34_HH__
