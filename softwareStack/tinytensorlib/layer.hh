/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * This file contains layer argument data structures.
 *
 */

#ifndef __LAYER_HH__
#define __LAYER_HH__

#include <string>

#ifdef CNM
#include "cnm.h"
#endif

using namespace std;

///////////////////////////
// Forward declarations. //
///////////////////////////

struct LAYER_ARG_BASE;
struct CONV_LAYER_ARGS;
struct FC_LAYER_ARGS;
struct POOL_LAYER_ARGS;
struct FLATTEN_LAYER_ARGS;
struct END_RESIDUAL_LAYER_ARGS;
struct DWCONV_LAYER_ARGS;
#ifdef CNM
struct CONV_CNM_FORMAT_ARGS;
struct FC_CNM_FORMAT_ARGS;
struct ENDRES_CNM_FORMAT_ARGS;
struct CNM_2_EIGEN;
#endif

/////////////////////////////
// Operation enumerations. //
/////////////////////////////

enum args_t
{
    BASE_ARGS_TYPE,
    CONV_ARGS_TYPE,
    FC_ARGS_TYPE,
    POOL_ARGS_TYPE,
    FLATTEN_ARGS_TYPE,
    END_RESIDUAL_ARGS_TYPE,
    DWCONV_ARGS_TYPE
};

enum pool_ops_t
{
    NO_POOL_TYPE,
    MAX_POOL_TYPE,
    AVG_POOL_TYPE
};

enum norm_ops_t
{
    NO_NORM_TYPE,
    BATCH_NORM_TYPE,
    LRN_NORM_TYPE
};

enum act_ops_t
{
    NO_ACT_TYPE,
    RELU_ACT_TYPE,
    SIGMOID_ACT_TYPE,
    SOFTMAX_ACT_TYPE,
    RELU6_ACT_TYPE
};

enum buffer_t
{
    SINGLE_BUFFER_TYPE,
    PING_PONG_BUFFER_TYPE
};

///////////////////////
// Helper functions. //
///////////////////////

// Helper functions to allocate and initialize all input or output data
// structures.  Inputs generate with random data while output generate with
// zeroed data.
inline void
generateIODataStructures(TB_Vector * target, const int T_x, const int in_s,
    bool setRandom)
{
    for (int i = 0; i < T_x; i++) {
        target[i] = TB_Vector(in_s);
        if (setRandom) {
            target[i].setRandom();
            target[i] = target[i]* Eigen::half(2) - Eigen::half(1);
        } else {
            target[i].setZero();
        }
    }
    return;
}

inline void
generateIODataStructures(TB_Matrix3D * target, const int T_x, const int in_c,
    const int in_h, const int in_w, bool setRandom)
{
    for (int i = 0; i < T_x; i++) {
        target[i] = TB_Matrix3D(in_c, in_h, in_w);
        if (setRandom) {
            target[i].setRandom();
            target[i] = target[i]* Eigen::half(2) - Eigen::half(1);
        } else {
            target[i].setZero();
        }
    }

    return;
}

#ifdef CNM

void initialize_3D_Matrix(TB_Matrix3D tensor, uint64_t* matrix){

    uint size_64B = tensor.dimension(0)*div_ceil(tensor.dimension(1)*tensor.dimension(2), SIMD_WIDTH)*GRF_64B;
    uint round_size = size_64B*WORDS_PER_64B;
    if (round_size != tensor.dimension(0)*tensor.dimension(1)*tensor.dimension(2))
        std::cout << "Warning: tensor.dimension(0)xtensor.dimension(1)*tensor.dimension(2) is not aligned with WORDS_PER_64B\n";
    
    half_float::half* m_half = new half_float::half[round_size];
    uint m = tensor.dimension(0);
    uint n = tensor.dimension(1)*tensor.dimension(2);
    
    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < n; j++) {
            uint j_w = j%tensor.dimension(2);
            uint j_h = j/tensor.dimension(2);
            m_half[i*(round_size/m)+j] = half_float::half(tensor(i,j_h, j_w));
            //std::cout << "input (" << i << "," << j_h << "," << j_w << ") =" << half_float::half(tensor(i,j_h, j_w)) << std::endl;
            // m_half[i*(round_size/m)+j] = half_float::half(1.0); // For testing purposes fill with 1.0
            // m_half[i*(round_size/m)+j] = half_float::half(float(i*n)/64+float(j)/64);   // For testing purposes fill with sequential values
        }
        for (uint j = n; j < round_size/m; j++) {
            m_half[i*(round_size/m)+j] = half_float::half(0.0);
        }
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        matrix[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            matrix[i] |= (uint64_t(m_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void initialize_2D_Matrix(TB_Matrix2D tensor, uint64_t* matrix){
    
    uint size_64B = tensor.dimension(0)*div_ceil(tensor.dimension(1), SIMD_WIDTH)*GRF_64B;
    uint round_size = size_64B*WORDS_PER_64B;
    if (round_size != tensor.dimension(0)*tensor.dimension(1))
        std::cout << "Warning: tensor.dimension(0)xtensor.dimension(1)*tensor.dimension(2) is not aligned with WORDS_PER_64B\n";
    
    half_float::half* m_half = new half_float::half[round_size];
    uint m = tensor.dimension(0);
    uint n = tensor.dimension(1);
    
    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < n; j++) {
            m_half[i*(round_size/m)+j] = half_float::half(tensor(i,j));
            //std::cout << "input (" << i << "," << j_h << "," << j_w << ") =" << half_float::half(tensor(i,j_h, j_w)) << std::endl;
            // m_half[i*(round_size/m)+j] = half_float::half(1.0); // For testing purposes fill with 1.0
            // m_half[i*(round_size/m)+j] = half_float::half(float(i*n)/64+float(j)/64);   // For testing purposes fill with sequential values
        }
        for (uint j = n; j < round_size/m; j++) {
            m_half[i*(round_size/m)+j] = half_float::half(0.0);
        }
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        matrix[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            matrix[i] |= (uint64_t(m_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void initialize_vector (TB_Vector tensor, uint64_t* v) {

    uint size_64B = div_ceil(tensor.dimension(0), SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != tensor.dimension(0))
        std::cout << "Warning: Vector size is not aligned with WORDS_PER_64B\n";

    half_float::half* v_half = new half_float::half[round_size];

    // Generate random half precision values
    for (uint i = 0; i < tensor.dimension(0); i++) {
        v_half[i] = half_float::half(tensor(i));
    }
    for (uint i = tensor.dimension(0); i < round_size; i++) {
        v_half[i] = half_float::half(0.0);
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        v[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            v[i] |= (uint64_t(v_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void initialize_vectors (TB_Matrix3D tensor, uint64_t* v) {

    uint size_64B = div_ceil(tensor.dimension(0)*tensor.dimension(1)*tensor.dimension(2), SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != tensor.dimension(0)*tensor.dimension(1)*tensor.dimension(2))
        std::cout << "Warning: Vector size is not aligned with WORDS_PER_64B\n";

    half_float::half* v_half = new half_float::half[round_size];

    for (uint i = 0; i < tensor.dimension(0); i++) {
        for(uint j = 0; j < tensor.dimension(1)*tensor.dimension(2); j++){
            uint j_w = j%tensor.dimension(2);
            uint j_h = j/tensor.dimension(2);
            v_half[i*(round_size/tensor.dimension(0))+j] = half_float::half(tensor(i, j_h, j_w));
            //v_half[i*(round_size/tensor.dimension(0))+j] = half_float::half(1.0);
        }  
    
        for (uint j = tensor.dimension(1)*tensor.dimension(2); j < round_size/tensor.dimension(0); j++) {
            v_half[i*(round_size/tensor.dimension(0))+j] = half_float::half(0.0);
        }
    }
    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        v[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            v[i] |= (uint64_t(v_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void CnM2Eigen (uint64_t* res, TB_Matrix3D& tensor) {
    uint co = tensor.dimension(0);
    uint ho = tensor.dimension(1);
    uint wo = tensor.dimension(2);
    uint size_64B = co * div_ceil(ho*wo, SIMD_WIDTH)*GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* m_half = new half_float::half[round_size];
    int h,w ;

    for (uint i = 0; i < size_64B; i++) {                  
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
            m_half[WORDS_PER_64B*i+j] = val1;
        }
    }
    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < ho*wo; j++) {
            h = j/wo;
            w = j%wo;
            tensor(i,h,w) = Eigen::half(m_half[i*(round_size/co)+j]);
        }
    }
}

// @ToDo ketu eshte funksioni i modifikuar qe ta mbushi vetem me 0, po qe shife si mund ta modifikosh dhe te riperdoresh ate nga cnmlib
void fill_vector (std::mt19937& gen, uint64_t* v, uint numVectors, uint vectorDims) {

    std::uniform_real_distribution<float> rng(-10.0, 10.0);
    uint size_64B = div_ceil(numVectors*vectorDims, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != numVectors*vectorDims)
        std::cout << "Warning: numVectors*vectorDims is not aligned with WORDS_PER_64B\n";

    half_float::half* v_half = new half_float::half[round_size];

    // Generate random half precision values
    for (uint i = 0; i < numVectors*vectorDims; i++) {
        v_half[i] = half_float::half(0.0);  //rng(gen);
        // v_half[i] = half_float::half(1.0);  // For testing purposes fill with 1.0
    }
    for (uint i = numVectors*vectorDims; i < round_size; i++) {
        v_half[i] = half_float::half(0.0);
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        v[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            v[i] |= (uint64_t(v_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

#endif


/////////////////////////////
// Layer argument structs. //
/////////////////////////////
#ifdef CNM
typedef struct CONV_CNM_FORMAT_ARGS
{
    uint size_64B_input;
    uint round_size_input;
    uint size_64B_weights;
    uint round_size_weights;
    uint size_64B_bias;
    uint round_size_bias;
    uint size_64B_res;
    uint round_size_res;

    uint64_t* CNM_input;
    uint64_t* CNM_weights;
    uint64_t* CNM_bias;
    uint64_t* CNM_res;
    uint64_t* CNM_res_check;

    bool    relu;

    std::mt19937 gen;   

    //Constructor
    CONV_CNM_FORMAT_ARGS(uint HI, uint WI, uint CI, uint K, uint CO, uint STRIDE, uint PADDING, bool RELU, uint HO, uint WO):
    size_64B_input(CI*div_ceil(HI*WI, SIMD_WIDTH) * GRF_64B), round_size_input(size_64B_input*WORDS_PER_64B), size_64B_weights(CO*div_ceil(K*K*CI, SIMD_WIDTH) * GRF_64B), round_size_weights(size_64B_weights * WORDS_PER_64B), size_64B_bias(div_ceil(CO, SIMD_WIDTH) * GRF_64B), round_size_bias(size_64B_bias * WORDS_PER_64B), size_64B_res(CO*div_ceil(HO*WO, SIMD_WIDTH) * GRF_64B), round_size_res(size_64B_res * WORDS_PER_64B), gen(SEED), relu(RELU)
    {
        CNM_input = new uint64_t[size_64B_input];
        CNM_weights = new uint64_t[size_64B_weights];
        CNM_bias = new uint64_t[size_64B_bias];

        fill_vector(gen, CNM_bias, 1, CO); 

        CNM_res = new uint64_t[size_64B_res];   
        CNM_res_check = new uint64_t[size_64B_res]; 
    }

    //Destructor
     ~CONV_CNM_FORMAT_ARGS() {
        delete[] CNM_input;  // Free the allocated memory
        delete[] CNM_weights;  // Free the allocated memory
        delete[] CNM_bias;  // Free the allocated memory
        delete[] CNM_res;  // Free the allocated memory
        delete[] CNM_res_check;  // Free the allocated memory
    }

} conv_cnm_format_args;

typedef struct FC_CNM_FORMAT_ARGS
{
    uint size_64B_v1;
    uint round_size_v1;
    uint size_64B_m2;
    uint round_size_m2;
    uint size_64B_res;
    uint round_size_res;

    uint64_t* CNM_v1;
    uint64_t* CNM_m2;
    uint64_t* CNM_res;
    uint64_t* CNM_res_check;

    std::mt19937 gen;

    //Constructor
    FC_CNM_FORMAT_ARGS(uint M, uint N):
    size_64B_v1(div_ceil(M, SIMD_WIDTH)*GRF_64B), round_size_v1(size_64B_v1*WORDS_PER_64B), size_64B_m2(M*div_ceil(N, SIMD_WIDTH)*GRF_64B), round_size_m2(size_64B_m2*WORDS_PER_64B), size_64B_res(div_ceil(N, SIMD_WIDTH)*GRF_64B), round_size_res(size_64B_res*WORDS_PER_64B), gen(SEED)
    {
        CNM_v1 = new uint64_t[size_64B_v1];
        CNM_m2 = new uint64_t[size_64B_m2];
        CNM_res = new uint64_t[size_64B_res];
        CNM_res_check = new uint64_t[size_64B_res];
    }

    //Destructor
    ~FC_CNM_FORMAT_ARGS(){
        delete[] CNM_v1;
        delete[] CNM_m2;
        delete[] CNM_res;
        delete[] CNM_res_check;
    }


} fc_cnm_format_args;

typedef struct ENDRES_CNM_FORMAT_ARGS
{
    uint size_64B;
    uint round_size;

    uint64_t* CNM_v1;
    uint64_t* CNM_v2;
    uint64_t* CNM_res;
    uint64_t* CNM_res_check;

    std::mt19937 gen;
    
    //Constructor
    ENDRES_CNM_FORMAT_ARGS(uint NV, uint ND):
    size_64B(div_ceil(NV*ND,SIMD_WIDTH)*GRF_64B), round_size(size_64B*WORDS_PER_64B), gen(SEED)
    {
        CNM_v1 = new uint64_t[size_64B];
        CNM_v2 = new uint64_t[size_64B];
        CNM_res = new uint64_t[size_64B];
        CNM_res_check = new uint64_t[size_64B];
    }

    //Destructor
    ~ENDRES_CNM_FORMAT_ARGS() {
        delete[] CNM_v1;
        delete[] CNM_v2;
        delete[] CNM_res;
        delete[] CNM_res_check;
    }

} endres_cnm_format_args;

typedef struct CNM_2_EIGEN
{
    
    TB_Matrix3D CnM2Eigen_3Dinput;
    TB_Matrix3D CnM2Eigen_3Doutput;

    TB_Vector CnM2Eigen_1Dinput;
    TB_Vector CnM2Eigen_1Doutput;
    
    //Constructor
    CNM_2_EIGEN(uint CI, uint HI, uint WI, uint CO, uint HO, uint WO, uint V_IN, uint V_OUT):
        CnM2Eigen_3Dinput(CI, HI, WI),
        CnM2Eigen_3Doutput(CO, HO, WO),
        CnM2Eigen_1Dinput(V_IN),
        CnM2Eigen_1Doutput(V_OUT){}

    //Destructor
    ~CNM_2_EIGEN() {
    }

} cnm_2_eigen;
#endif

// Base struct for layer arguments.
typedef struct LAYER_ARG_BASE
{
    // Metadata.
    args_t args_type;
    const buffer_t buffer_type;
    const int layer_n;
    const int T_x; // Number of inferences.
    const bool isFirstLayer;
    const bool isLastLayer;
    int cond_var_prv_lyr_idx;
    int cond_var_nxt_lyr_idx;
    int cnt_mutex_prv_lyr_idx;
    int cnt_mutex_nxt_lyr_idx;
    string name;
    uint8_t pong_idx; // Select ping/pong if using this kind of buffer.

    // Constructor.
    LAYER_ARG_BASE(const int lyr, const int inf,
        const bool first, const bool last, const buffer_t b_t) :
        args_type(BASE_ARGS_TYPE), buffer_type(b_t), layer_n(lyr), T_x(inf),
        isFirstLayer(first), isLastLayer(last),
        cond_var_prv_lyr_idx(0), cond_var_nxt_lyr_idx(0),
        cnt_mutex_prv_lyr_idx(0), cnt_mutex_nxt_lyr_idx(0), name(""),
        pong_idx(0)
    {}

    ~LAYER_ARG_BASE() {}
} layer_arg_base;

// Struct to hold 1-D (vector) input, associated metadata.
struct LAYER_ARG_1D_IN
{
    // Metadata, data structures.
    const int input_size;
    TB_Vector * input;    // Used to point to the input being used.

    // Constructor -- note inference count is held in base layer arg struct.
    LAYER_ARG_1D_IN(const int inf, bool first, const int in_s) :
        input_size(in_s)
    {
        // Initialize inputs if first layer.
        if (first) {
            input = new TB_Vector[inf];
            generateIODataStructures(input, inf, in_s, true);
        }
    }

    ~LAYER_ARG_1D_IN() {}
};

// Struct to hold 1-D (vector) output, associated metadata.
struct LAYER_ARG_1D_OUT
{
    // Metadata and data structures.
    const int output_size;
    TB_Vector * output;   // Used to point to the output being used.
    const norm_ops_t normalization;
    const act_ops_t activation;

    // Constructor -- note inference count is held in base layer arg struct.
    LAYER_ARG_1D_OUT(const int inf, bool last, const int out_s,
        const norm_ops_t norm, const act_ops_t act) :
        output_size(out_s), normalization(norm), activation(act)
    {
        // Initialize output if last layer.
        if (last) {
            output = new TB_Vector[inf];
            generateIODataStructures(output, inf, out_s, false);
        }
    }

    ~LAYER_ARG_1D_OUT() {}
};

// Struct to hold 3-D (matrix) input and associated metadata.
struct LAYER_ARG_3D_IN
{
    // Metadata.
    const int input_c;
    const int input_h;
    const int input_w;
    const int input_size;

    // Data structures, with ping-pong index to prevent buffer blocking.
    TB_Matrix3D * input;  // Used to point to the input being used.

    // Constructor -- note inference count is held in base layer arg struct.
    LAYER_ARG_3D_IN(const int inf, bool first, const int in_c,
        const int in_h, const int in_w) :
        input_c(in_c), input_h(in_h), input_w(in_w),
        input_size(in_c * in_h * in_w)
    {
        // Initialize input if first layer.
        if (first) {
            input = new TB_Matrix3D[inf];
            generateIODataStructures(input, inf, in_c, in_h, in_w, true);
        }
    }

    ~LAYER_ARG_3D_IN() {}
};

// Struct to hold 3-D (matrix) output and associated metadata.
struct LAYER_ARG_3D_OUT
{
    // Metadata.
    const int output_c;
    const int output_h;
    const int output_w;
    const int output_size;

    // Data structures, with ping-pong index to prevent buffer blocking.
    TB_Matrix3D * output; // Used to point to the output being used.

    // If the layer will use normalization or activation functions.
    const norm_ops_t normalization;
    const act_ops_t activation;

    // Constructor -- note inference count is held in base layer arg struct.
    LAYER_ARG_3D_OUT(const int inf, bool last, const int out_c, const int out_h,
        const int out_w, const norm_ops_t norm, const act_ops_t act) :
        output_c(out_c), output_h(out_h), output_w(out_w),
        output_size(out_c * out_h * out_w), normalization(norm), activation(act)
    {
        // Initialize inputs/output if first/last layer.
        if (last) {
            output = new TB_Matrix3D[inf];
            generateIODataStructures(output, inf, out_c, out_h, out_w, false);
        }
    }

    ~LAYER_ARG_3D_OUT() {}
};

struct CONV_LAYER_ARGS : 
    public LAYER_ARG_BASE, LAYER_ARG_3D_IN, LAYER_ARG_3D_OUT 
#ifdef CNM
    , CONV_CNM_FORMAT_ARGS, CNM_2_EIGEN
#endif
{
    // Convolution variables.
    const int kernel_h;     // Kernel height.
    const int kernel_w;     // Kernel width.
    const int kernel_c;     // Kernel channels.
    const int kernel_size;  // Flattened kernel size.
    const int n_filters;    // Number of filters/kernels.
    const int stride;       // Stride over input.
    const int padding;      // Are we padding the input?
    PaddingType padding_type;

    // im2col transformed kernels.
    TB_Matrix2D weights;

    // Constructor.
    CONV_LAYER_ARGS(const int lyr, const int inf, bool first,
        bool last, buffer_t b_t, const int in_h, const int in_w,
        const int in_c, const int k_h, const int k_w, const int n_f,
        const int stri,
        const int pad, norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_3D_IN(inf, first, in_c, in_h, in_w),
        LAYER_ARG_3D_OUT(inf, last, n_f, ((in_h - k_h + 2 * pad) / stri + 1),
            ((in_w - k_w + 2 * pad) / stri + 1), norm, act),
#ifdef CNM
        CONV_CNM_FORMAT_ARGS(in_h, in_w, in_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_h - k_h + 2 * pad) / stri + 1), ((in_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
        CNM_2_EIGEN(in_c, in_h, in_w, n_f, ((in_h - k_h + 2 * pad) / stri + 1), ((in_w - k_w + 2 * pad) / stri + 1), 0, 0),
#endif
        kernel_h(k_h), kernel_w(k_w), kernel_c(in_c),
        kernel_size(k_w * k_h * in_c), n_filters(n_f), stride(stri),
        padding(pad)
    {
        args_type = CONV_ARGS_TYPE;
        name = "Conv";
        padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

        // Initialize weights.
        weights = TB_Matrix2D(k_h * k_w * in_c, n_f);
        weights.setRandom();
        weights = weights* Eigen::half(2) - Eigen::half(1);
#ifdef CNM
        initialize_2D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO)
        if(first)
            initialize_3D_Matrix(*LAYER_ARG_3D_IN::input, CONV_CNM_FORMAT_ARGS::CNM_input);
#endif
    }

    template<typename T>
    CONV_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
        T & in_lyr, const int k_h, const int k_w,
        const int n_f, const int stri,

        const int pad, norm_ops_t norm, act_ops_t act);

    // template<typename T>
    // CONV_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    //     T & in_lyr, const int k_h, const int k_w,
    //     const int n_f, const int stri,

    //     const int pad, norm_ops_t norm, act_ops_t act);

    // Destructor.
    ~CONV_LAYER_ARGS() {}
};

struct FC_LAYER_ARGS : public LAYER_ARG_BASE, LAYER_ARG_1D_IN, LAYER_ARG_1D_OUT
#ifdef CNM
 , FC_CNM_FORMAT_ARGS, CNM_2_EIGEN
#endif
{
    // Fully connected variables.
    const int weights_h;    // Weights matrix height.
    const int weights_w;    // Weights matrix width.
    TB_Matrix2D weights;    // Weights matrix.

    // Constructor and destructor.
    FC_LAYER_ARGS(const int lyr, const int inf, bool first,
        bool last, buffer_t b_t, const int in_s,
        const int out_s, norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_1D_IN(inf, first, in_s),
        LAYER_ARG_1D_OUT(inf, last, out_s, norm, act),
#ifdef CNM
    FC_CNM_FORMAT_ARGS(in_s, out_s),
    CNM_2_EIGEN(0, 0, 0, 0, 0, 0, in_s, out_s), 
#endif 
        weights_h(in_s), weights_w(out_s)
    {
        args_type = FC_ARGS_TYPE;
        name = "Dense";

        // Initialize weights.
        weights = TB_Matrix2D(in_s, out_s);
        weights.setRandom();
        weights = weights* Eigen::half(2) - Eigen::half(1);
#ifdef CNM
        if(first)
            initialize_vector(*LAYER_ARG_1D_IN::input, FC_CNM_FORMAT_ARGS::CNM_v1);
        initialize_2D_Matrix(weights, FC_CNM_FORMAT_ARGS::CNM_m2);
#endif
    }

    template<typename T>
    FC_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
        T & in_lyr,
        const int out_s, norm_ops_t norm, act_ops_t act);    

    // template<typename T>
    // FC_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    //     T & in_lyr,
    //     const int out_s, norm_ops_t norm, act_ops_t act);

    ~FC_LAYER_ARGS() {}
};

struct POOL_LAYER_ARGS :
    public LAYER_ARG_BASE, LAYER_ARG_3D_IN, LAYER_ARG_3D_OUT
#ifdef CNM
    , CNM_2_EIGEN
#endif
{
    // Fully connected variables.
    const int pool_h;       // Pool height.
    const int pool_w;       // Pool width.
    const int pool_f;       // Pooling factor.
    const int stride;       // Pool stride.
    pool_ops_t pool_type;      // What kind of pooling is this layer?

    // Constructor and destructor.
    POOL_LAYER_ARGS(const int lyr, const int inf, bool first,
        bool last, buffer_t b_t,
        const int in_h, const int in_w, const int in_c,
        const int p_h, const int p_w, const int p_f, const int stri,
        pool_ops_t p_type, norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_3D_IN(inf, first, in_c, in_h, in_w),
        LAYER_ARG_3D_OUT(inf, last, in_c, in_h / p_h, in_w / p_w, norm, act),
#ifdef CNM
        CNM_2_EIGEN(in_c, in_h, in_w, in_c, in_h / p_h, in_w / p_w, 0, 0), 
#endif        
        pool_h(p_h), pool_w(p_w), pool_f(p_f), stride(stri), pool_type(p_type)
    {
        args_type = POOL_ARGS_TYPE;
        name = "Pool";
    }

    template<typename T>
    POOL_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t, T & in_lyr,
        const int p_d, const int p_f, const int stri, pool_ops_t p_type,
        norm_ops_t norm, act_ops_t act);

    // template<typename T>
    // POOL_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    //     T & in_lyr, const int p_d, const int p_f, const int stri,
    //     pool_ops_t p_type, norm_ops_t norm, act_ops_t act);

    ~POOL_LAYER_ARGS() {}
};

struct FLATTEN_LAYER_ARGS :
    public LAYER_ARG_BASE, LAYER_ARG_3D_IN, LAYER_ARG_1D_OUT
#ifdef CNM
    , CNM_2_EIGEN
#endif    
{
    // Constructor and destructor.
    FLATTEN_LAYER_ARGS(const int lyr, const int inf, 
        bool first, bool last, buffer_t b_t,
        const int in_h, const int in_w, const int in_c,
        norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_3D_IN(inf, first, in_c, in_h, in_w),
        LAYER_ARG_1D_OUT(inf, last, in_c * in_h * in_w, norm, act)
#ifdef CNM
        , CNM_2_EIGEN(in_c, in_h, in_w, 0, 0, 0, 0, in_c*in_h*in_w)        
#endif     
    {
        args_type = FLATTEN_ARGS_TYPE;
        name = "Flatten";
    }

    template<typename T>
    FLATTEN_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t, T & in_lyr);

    // template<typename T>
    // FLATTEN_LAYER_ARGS(const int lyr, const int inf, 
    //     buffer_t b_t, T & in_lyr);

    ~FLATTEN_LAYER_ARGS() {}
};

// Addition layer for residual block.
struct END_RESIDUAL_LAYER_ARGS :
    public LAYER_ARG_BASE, LAYER_ARG_3D_IN, LAYER_ARG_3D_OUT
#ifdef CNM
    , ENDRES_CNM_FORMAT_ARGS, CNM_2_EIGEN
#endif
{
    // Used to point to input being combined.
    TB_Matrix3D * residual;

    // Constructor and destructor.
    END_RESIDUAL_LAYER_ARGS(const int lyr, const int inf, 
        bool first, bool last, buffer_t b_t,
        const int in_h, const int in_w, const int in_c,
        norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_3D_IN(inf, first, in_c, in_h, in_w),
        LAYER_ARG_3D_OUT(inf, last, in_c, in_h, in_w, norm, act)
#ifdef CNM
        , ENDRES_CNM_FORMAT_ARGS(in_c*in_h, in_w), //I am creating the vectors out of the 3D tensor where a vector is each row in the tensor, so in total there are in_c*in_h vectors of size in_w - the reason I did this was to have to iterate through the last dimension (rows)
        CNM_2_EIGEN(in_c, in_h, in_w, in_c, in_h, in_w, 0, 0)             
#endif
    {
        args_type = END_RESIDUAL_ARGS_TYPE;
        name = "Residual";

        // Initialize residual if first layer (not that this makes much sense).
        if (first) {
            residual = new TB_Matrix3D[inf];
            generateIODataStructures(residual, inf, in_c, in_h, in_w, true);
        }
#ifdef CNM
        if (first){
            initialize_vectors(*LAYER_ARG_3D_IN::input, ENDRES_CNM_FORMAT_ARGS::CNM_v1);
            initialize_vectors(*residual, ENDRES_CNM_FORMAT_ARGS::CNM_v2);
        }
#endif
    }

    template<typename T>
    END_RESIDUAL_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
        T & in_lyr, norm_ops_t norm, act_ops_t act);

    // template<typename T>
    // END_RESIDUAL_LAYER_ARGS(const int lyr, const int inf, 
    //     buffer_t b_t, T & in_lyr, norm_ops_t norm, act_ops_t act);

    ~END_RESIDUAL_LAYER_ARGS() {}
};

struct DWCONV_LAYER_ARGS :
    public LAYER_ARG_BASE, LAYER_ARG_3D_IN, LAYER_ARG_3D_OUT
#ifdef CNM
    , CONV_CNM_FORMAT_ARGS, CNM_2_EIGEN
#endif
{
    // Convolution variables.
    const int kernel_h;     // Kernel height.
    const int kernel_w;     // Kernel width.
    const int kernel_c;     // Kernel channels.
    const int kernel_size;  // Flattened kernel size.
    const int n_filters;    // Number of filters/kernels.
    const int stride;       // Stride over input.
    const int padding;      // Are we padding the input?
    PaddingType padding_type;

    // Kernels organized by channel, dim, dim.
    TB_Matrix3D weights;

    // Constructor.
    DWCONV_LAYER_ARGS(const int lyr, const int inf, bool first,
        bool last, buffer_t b_t, const int in_h, const int in_w,
        const int in_c, const int k_h, const int k_w, const int n_f,
        const int stri, const int pad, norm_ops_t norm, act_ops_t act) :
        LAYER_ARG_BASE(lyr, inf, first, last, b_t),
        LAYER_ARG_3D_IN(inf, first, in_c, in_h, in_w),
        LAYER_ARG_3D_OUT(inf, last, n_f, ((in_h - k_h + 2 * pad) / stri + 1),
            ((in_w - k_w + 2 * pad) / stri + 1), norm, act),
#ifdef CNM
        CONV_CNM_FORMAT_ARGS(in_h, in_w, in_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_h - k_h + 2 * pad) / stri + 1), ((in_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
        CNM_2_EIGEN(in_c, in_h, in_w, n_f, ((in_h - k_h + 2 * pad) / stri + 1),((in_w - k_w + 2 * pad) / stri + 1), 0, 0), 
#endif
        kernel_h(k_h), kernel_w(k_w), kernel_c(in_c),
        kernel_size(k_w * k_h * in_c), n_filters(n_f), stride(stri),
        padding(pad)
    {
        args_type = DWCONV_ARGS_TYPE;
        name = "DWConv";
        padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

        // Initialize weights.
        weights = TB_Matrix3D(n_f, k_h, k_w);
        weights.setRandom();
        weights = weights* Eigen::half(2) - Eigen::half(1);
// Needs to be fixed        
// #ifdef CNM
//         initialize_3D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO) 
//         if(first)
//             initialize_3D_Matrix(*LAYER_ARG_3D_IN::input, CONV_CNM_FORMAT_ARGS::CNM_input);
// #endif
    }

    template<typename T>
    DWCONV_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t, T & in_lyr,
        const int k_h, const int k_w, const int n_f, const int stri,
        const int pad, norm_ops_t norm, act_ops_t act);

    // template<typename T>
    // DWCONV_LAYER_ARGS(const int lyr, const int inf, 
    //     buffer_t b_t, T & in_lyr, const int k_h, const int k_w, const int n_f,
    //     const int stri, const int pad, norm_ops_t norm, act_ops_t act);

    // Destructor.
    ~DWCONV_LAYER_ARGS() {}
};

///////////////////////////////
// Alternative constructors. //
///////////////////////////////

template<typename T>
CONV_LAYER_ARGS::CONV_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    T & in_lyr, const int k_h, const int k_w, const int n_f, const int stri,
    const int pad, norm_ops_t norm, act_ops_t act) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w),
    LAYER_ARG_3D_OUT(inf, false, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1),
        ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), norm, act),
#ifdef CNM
        CONV_CNM_FORMAT_ARGS(in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
        CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), 0, 0), 
#endif
    kernel_h(k_h), kernel_w(k_w), kernel_c(in_lyr.output_c),
    kernel_size(k_w * k_h * in_lyr.output_c), n_filters(n_f), stride(stri),
    padding(pad)
{
    args_type = CONV_ARGS_TYPE;
    name = "Conv";
    padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

    // Initialize weights.
    weights = TB_Matrix2D(k_h * k_w * in_lyr.output_c, n_f);
    weights.setRandom();
    weights = weights* Eigen::half(2) - Eigen::half(1);
#ifdef CNM
    initialize_2D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO)
#endif
}

// template<typename T>
// CONV_LAYER_ARGS::CONV_LAYER_ARGS(const int lyr, const int inf, 
//     buffer_t b_t, T & in_lyr, const int k_h, const int k_w, const int n_f,
//     const int stri,
//     const int pad, norm_ops_t norm, act_ops_t act) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w),
//     LAYER_ARG_3D_OUT(inf, false, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1),
//         ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), norm, act),
// #ifdef CNM
//     CONV_CNM_FORMAT_ARGS(in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
//     CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1),0, 0), 
// #endif
//     kernel_h(k_h), kernel_w(k_w), kernel_c(in_lyr.output_c),
//     kernel_size(k_w * k_h * in_lyr.output_c), n_filters(n_f), stride(stri),
//     padding(pad)
// {
//     args_type = CONV_ARGS_TYPE;
//     name = "Conv";
//     padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

//     // Initialize weights.
//     weights = TB_Matrix2D(k_h * k_w * in_lyr.output_c, n_f);
//     weights.setRandom();
//     weights = weights* Eigen::half(2) - Eigen::half(1);
// #ifdef CNM
//     initialize_2D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO)
// #endif
// }

template<typename T>
FC_LAYER_ARGS::FC_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    T & in_lyr,
    const int out_s, norm_ops_t norm, act_ops_t act) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_1D_IN(inf, false, in_lyr.output_size),
    LAYER_ARG_1D_OUT(inf, false, out_s, norm, act),
#ifdef CNM
    FC_CNM_FORMAT_ARGS(in_lyr.output_size, out_s),
    CNM_2_EIGEN(0, 0, 0, 0, 0, 0, in_lyr.output_size, out_s), 
#endif 
    weights_h(in_lyr.output_size), weights_w(out_s)
{
    args_type = FC_ARGS_TYPE;
    name = "Dense";

    // Initialize weights.
    weights = TB_Matrix2D(in_lyr.output_size, out_s);
    weights.setRandom();
    weights = weights* Eigen::half(2) - Eigen::half(1);
#ifdef CNM
    initialize_2D_Matrix(weights, FC_CNM_FORMAT_ARGS::CNM_m2);
#endif
}

// template<typename T>
// FC_LAYER_ARGS::FC_LAYER_ARGS(const int lyr, const int inf, 
//     buffer_t b_t, T & in_lyr,
//     const int out_s, norm_ops_t norm, act_ops_t act) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_1D_IN(inf, false, in_lyr.output_size),
//     LAYER_ARG_1D_OUT(inf, false, out_s, norm, act),
// #ifdef CNM
//     FC_CNM_FORMAT_ARGS(in_lyr.output_size, out_s),
//     CNM_2_EIGEN(0, 0, 0, 0, 0, 0, in_lyr.output_size, out_s),
// #endif 
//     weights_h(in_lyr.output_size),
//     weights_w(out_s)
// {
//     args_type = FC_ARGS_TYPE;
//     name = "Dense";

//     // Initialize weights.
//     weights = TB_Matrix2D(in_lyr.output_size, out_s);
//     weights.setRandom();
//     weights = weights* Eigen::half(2) - Eigen::half(1);
// #ifdef CNM
//     initialize_2D_Matrix(weights, FC_CNM_FORMAT_ARGS::CNM_m2);
// #endif
// }

template<typename T>
POOL_LAYER_ARGS::POOL_LAYER_ARGS(const int lyr, const int inf, buffer_t b_t,
    T & in_lyr, const int p_d, const int p_f, const int stri,
    pool_ops_t p_type, norm_ops_t norm, act_ops_t act) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w),
    LAYER_ARG_3D_OUT(inf, false, in_lyr.output_c, in_lyr.output_h / p_d,
        in_lyr.output_w / p_d, norm, act),
#ifdef CNM
    CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, in_lyr.output_h / p_d, in_lyr.output_w / p_d, 0, 0), 
#endif        
    pool_h(p_d), pool_w(p_d), pool_f(p_f), stride(stri), pool_type(p_type)
{
    args_type = POOL_ARGS_TYPE;
    name = "Pool";
}

// template<typename T>
// POOL_LAYER_ARGS::POOL_LAYER_ARGS(const int lyr, const int inf, 
//     buffer_t b_t, T & in_lyr, const int p_d, const int p_f, const int stri,
//     pool_ops_t p_type, norm_ops_t norm, act_ops_t act) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w),
//     LAYER_ARG_3D_OUT(inf, false, in_lyr.output_c, in_lyr.output_h / p_d,
//         in_lyr.output_w / p_d, norm, act),
// #ifdef CNM
//     CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, in_lyr.output_h / p_d, in_lyr.output_w / p_d, 0, 0), 
// #endif        
//     pool_h(p_d), pool_w(p_d), pool_f(p_f), stride(stri), pool_type(p_type)
// {
//     args_type = POOL_ARGS_TYPE;
//     name = "Pool";
// }

template<typename T>
FLATTEN_LAYER_ARGS::FLATTEN_LAYER_ARGS(const int lyr, const int inf,
    buffer_t b_t, T & in_lyr) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w),
    LAYER_ARG_1D_OUT(inf, false,
        in_lyr.output_c * in_lyr.output_h * in_lyr.output_w, NO_NORM_TYPE,
        NO_ACT_TYPE)
#ifdef CNM
        , CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, 0, 0, 0, 0, in_lyr.output_c*in_lyr.output_h*in_lyr.output_w)        
#endif
{
    args_type = FLATTEN_ARGS_TYPE;
    name = "Flatten";
}

// template<typename T>
// FLATTEN_LAYER_ARGS::FLATTEN_LAYER_ARGS(const int lyr, const int inf,
//      buffer_t b_t, T & in_lyr) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w),
//     LAYER_ARG_1D_OUT(inf, false,
//         in_lyr.output_c * in_lyr.output_h * in_lyr.output_w, NO_NORM_TYPE,
//         NO_ACT_TYPE)
// #ifdef CNM
//         , CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, 0, 0, 0, 0, in_lyr.output_c*in_lyr.output_h*in_lyr.output_w)        
// #endif
// {
//     args_type = FLATTEN_ARGS_TYPE;
//     name = "Flatten";
// }

template<typename T>
END_RESIDUAL_LAYER_ARGS::END_RESIDUAL_LAYER_ARGS(const int lyr, const int inf,
    buffer_t b_t, T & in_lyr, norm_ops_t norm,
    act_ops_t act) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w),
    LAYER_ARG_3D_OUT(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w, norm, act)
#ifdef CNM
    , ENDRES_CNM_FORMAT_ARGS(in_lyr.output_c*in_lyr.output_h, in_lyr.output_w), //I am creating the vectors out of the 3D tensor where a vector is each row in the tensor, so in total there are in_c*in_h vectors of size in_w - the reason I did this was to have to iterate through the last dimension (rows)
    CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, 0, 0)              
#endif
{
    args_type = END_RESIDUAL_ARGS_TYPE;
    name = "Residual";
}

// template<typename T>
// END_RESIDUAL_LAYER_ARGS::END_RESIDUAL_LAYER_ARGS(const int lyr, const int inf,
//      buffer_t b_t, T & in_lyr,
//     norm_ops_t norm, act_ops_t act) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w),
//     LAYER_ARG_3D_OUT(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w, norm, act)
// #ifdef CNM
//     ,ENDRES_CNM_FORMAT_ARGS(in_lyr.output_c*in_lyr.output_h, in_lyr.output_w), //I am creating the vectors out of the 3D tensor where a vector is each row in the tensor, so in total there are in_c*in_h vectors of size in_w - the reason I did this was to have to iterate through the last dimension (rows)
//     CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, 0, 0)       
// #endif
// {
//     args_type = END_RESIDUAL_ARGS_TYPE;
//     name = "Residual";
// }

template<typename T>
DWCONV_LAYER_ARGS::DWCONV_LAYER_ARGS(const int lyr, const int inf,
    buffer_t b_t, T & in_lyr, const int k_h, const int k_w, const int n_f,
    const int stri, const int pad, norm_ops_t norm, act_ops_t act) :
    LAYER_ARG_BASE(lyr, inf, false, false, b_t),
    LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
        in_lyr.output_w),
    LAYER_ARG_3D_OUT(inf, false, n_f,
        ((in_lyr.output_h - k_h + 2 * pad) / stri + 1),
        ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), norm, act),
#ifdef CNM
        CONV_CNM_FORMAT_ARGS(in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
        CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), 0, 0), 
#endif
    kernel_h(k_h), kernel_w(k_w), kernel_c(in_lyr.output_c),
    kernel_size(k_w * k_h * in_lyr.output_c), n_filters(n_f), stride(stri),
    padding(pad)
{
    args_type = DWCONV_ARGS_TYPE;
    name = "DWConv";
    padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

    // Initialize weights.
    weights = TB_Matrix3D(n_f, k_h, k_w);
    weights.setRandom();
    weights = weights* Eigen::half(2) - Eigen::half(1);
// Needs to be fixed    
// #ifdef CNM
//     initialize_3D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO)
// #endif
}

// template<typename T>
// DWCONV_LAYER_ARGS::DWCONV_LAYER_ARGS(const int lyr, const int inf,
//      buffer_t b_t, T & in_lyr, const int k_h, const int k_w,
//     const int n_f, const int stri, const int pad, norm_ops_t norm,
//     act_ops_t act) :
//     LAYER_ARG_BASE(lyr, inf, false, false, b_t),
//     LAYER_ARG_3D_IN(inf, false, in_lyr.output_c, in_lyr.output_h,
//         in_lyr.output_w),
//     LAYER_ARG_3D_OUT(inf, false, n_f,
//         ((in_lyr.output_h - k_h + 2 * pad) / stri + 1),
//         ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), norm, act),
// #ifdef CNM
//         CONV_CNM_FORMAT_ARGS(in_lyr.output_h, in_lyr.output_w, in_lyr.output_c, k_h, n_f, stri, pad, (act==1)?1:0, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1)), //n_f = out_c act==1=RELU_ACT_TYPE
//         CNM_2_EIGEN(in_lyr.output_c, in_lyr.output_h, in_lyr.output_w, n_f, ((in_lyr.output_h - k_h + 2 * pad) / stri + 1), ((in_lyr.output_w - k_w + 2 * pad) / stri + 1), 0, 0), 
// #endif
//     kernel_h(k_h), kernel_w(k_w), kernel_c(in_lyr.output_c),
//     kernel_size(k_w * k_h * in_lyr.output_c), n_filters(n_f), stride(stri),
//     padding(pad)
// {
//     args_type = DWCONV_ARGS_TYPE;
//     name = "DWConv";
//     padding_type = (pad) ? PADDING_SAME : PADDING_VALID;

//     // Initialize weights.
//     weights = TB_Matrix3D(n_f, k_h, k_w);
//     weights.setRandom();
//     weights = weights* Eigen::half(2) - Eigen::half(1);
// // Needs to be fixed    
// // #ifdef CNM
// //     initialize_3D_Matrix(weights.shuffle(Eigen::array<int,2>{1,0}).eval(), CONV_CNM_FORMAT_ARGS::CNM_weights); //need to tranpose weights since in CnM weights (CO x K*K*CI) and here (K*K*CI x CO)
// // #endif
// }

///////////////
// Typedefs. //
///////////////

typedef CONV_LAYER_ARGS conv_layer_args;
typedef FC_LAYER_ARGS fc_layer_args;
typedef POOL_LAYER_ARGS pool_layer_args;
typedef FLATTEN_LAYER_ARGS flatten_layer_args;
typedef END_RESIDUAL_LAYER_ARGS end_residual_layer_args;
typedef DWCONV_LAYER_ARGS dwconv_layer_args;

#endif // __LAYER_HH__
