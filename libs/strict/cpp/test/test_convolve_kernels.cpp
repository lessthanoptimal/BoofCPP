#include "gtest/gtest.h"
#include "testing_utils.h"
#include "boofcv/convolve_kernels.h"

#include "boofcv/print_structures.h"

using namespace std;
using namespace boofcv;

class KernelBaseTest : public boofcv::BoofCPPTest {};

TEST_F(KernelBaseTest, sum) {
    Kernel1D<U8> kernel(1,{1,0,3,4,20});

    ASSERT_EQ(28,kernel.sum());

    Kernel1D<F32> kernel_F32(1,{1,-1.5f,3,4,-20.4f});

    ASSERT_FLOAT_EQ(-13.9,kernel_F32.sum());
}

TEST_F(KernelBaseTest, element_max) {
    Kernel1D<U8> kernel(1,{1,0,3,4,20});

    ASSERT_EQ(20,kernel.element_max());

    Kernel1D<F32> kernel_F32(1,{1,-1.5f,3,4,-20.4f});

    ASSERT_FLOAT_EQ(4,kernel_F32.element_max());
}

TEST_F(KernelBaseTest, element_min) {
    Kernel1D<U8> kernel(1,{1,0,3,4,20});

    ASSERT_EQ(0,kernel.element_min());

    Kernel1D<F32> kernel_F32(1,{1,-1.5f,3,4,-20.4f});

    ASSERT_FLOAT_EQ(-20.4f,kernel_F32.element_min());
}

TEST_F(KernelBaseTest, element_abs_max) {
    Kernel1D<U8> kernel(1,{1,0,3,4,20});

    ASSERT_EQ(20,kernel.element_abs_max());

    Kernel1D<F32> kernel_F32(1,{1,-1.5f,3,4,-20.4f});

    ASSERT_FLOAT_EQ(20.4f,kernel_F32.element_abs_max());
}

TEST_F(KernelBaseTest, element_abs_min) {
    Kernel1D<U8> kernel(1,{1,0,3,4,20});

    ASSERT_EQ(0,kernel.element_abs_min());

    Kernel1D<F32> kernel_F32(1,{1,-1.5f,3,4,-20.4f});

    ASSERT_FLOAT_EQ(1.0f,kernel_F32.element_abs_min());
}

class FactoryKernelTest : public boofcv::BoofCPPTest {};

TEST_F(FactoryKernelTest, table1D) {
    Kernel1D<U8> kernel_U8 = FactoryKernel::table1D<U8>(5);
    ASSERT_EQ(2,kernel_U8.offset);
    ASSERT_EQ(5,kernel_U8.width);
    for( uint32_t i = 0; i < 5; i++ ) {
        ASSERT_EQ(1,kernel_U8.at(i));
    }

    Kernel1D<F32> kernel_F32 = FactoryKernel::table1D<F32>(5);
    ASSERT_EQ(2,kernel_F32.offset);
    ASSERT_EQ(5,kernel_F32.width);
    for( uint32_t i = 0; i < 5; i++ ) {
        ASSERT_FLOAT_EQ(1,kernel_F32.at(i));
    }
}

TEST_F(FactoryKernelTest, mean1D) {
    Kernel1D<U8> kernel_U8 = FactoryKernel::mean1D<U8>(5);
    ASSERT_EQ(2,kernel_U8.offset);
    ASSERT_EQ(5,kernel_U8.width);
    for( uint32_t i = 0; i < 5; i++ ) {
        ASSERT_EQ(1,kernel_U8.at(i));
    }

    Kernel1D<F32> kernel_F32 = FactoryKernel::mean1D<F32>(5);
    ASSERT_EQ(2,kernel_F32.offset);
    ASSERT_EQ(5,kernel_F32.width);
    for( uint32_t i = 0; i < 5; i++ ) {
        ASSERT_FLOAT_EQ(0.2f,kernel_F32.at(i));
    }
}

TEST_F(FactoryKernelTest, normalize) {
    Kernel1D<U8> kernel_U8(1,{1,0,3,4,20});

    // can't normalize integer to sum up to one. it should do nothing
    FactoryKernel::normalize(kernel_U8);
    ASSERT_EQ(28,kernel_U8.sum());

    // normalizing only makes sense for kernels with all positive elements
    Kernel1D<F32> kernel_F32(1,{1,1.5f,3,4,20.4f});

    FactoryKernel::normalize(kernel_F32);
    ASSERT_FLOAT_EQ(1.0f,kernel_F32.sum());
    ASSERT_FLOAT_EQ(0.10033445f,kernel_F32.at(2));
}

TEST_F(FactoryKernelTest, sigmaForRadius) {
    ASSERT_DOUBLE_EQ(0.6,FactoryKernel::sigmaForRadius(1,0));
    ASSERT_DOUBLE_EQ(1.0,FactoryKernel::sigmaForRadius(2,0));
    ASSERT_DOUBLE_EQ(1.4,FactoryKernel::sigmaForRadius(3,0));
}

TEST_F(FactoryKernelTest, radiusForSigma) {
    ASSERT_EQ(1,FactoryKernel::radiusForSigma(0.6,0));
    ASSERT_EQ(2,FactoryKernel::radiusForSigma(1.0,0));
    ASSERT_EQ(3,FactoryKernel::radiusForSigma(1.4,0));
}

TEST_F(FactoryKernelTest, gaussian1D_S32) {
    Kernel1D<S32> kernel = FactoryKernel::gaussian1D<S32>(-1.0,5);

    // Should be symmetric
    for( uint32_t i = 0; i < 3; i++ ) {
        ASSERT_EQ(kernel.at(i),kernel.at(4-i));
    }
    // peak should be at the center
    ASSERT_EQ(kernel.element_max(),kernel.at(2));
    ASSERT_TRUE(kernel.at(2)>kernel.at(1) && kernel.at(2)>kernel.at(3));

    kernel = FactoryKernel::gaussian1D<S32>(-1.0,4);
    // Should be symmetric
    for( uint32_t i = 0; i < 2; i++ ) {
        ASSERT_EQ(kernel.at(i),kernel.at(3-i));
    }
    // peak should be at the center
    ASSERT_EQ(kernel.element_max(),kernel.at(2));
    ASSERT_EQ(kernel.at(1),kernel.at(2));
    ASSERT_TRUE(kernel.at(3)<kernel.at(2) && kernel.at(1)>kernel.at(0));
}

TEST_F(FactoryKernelTest, gaussian1D_F32) {
    Kernel1D<F32> kernel = FactoryKernel::gaussian1D<F32>(-1.0,5);

    // sum should add to one
    ASSERT_FLOAT_EQ(1.0f,kernel.sum());
    // Should be symmetric
    for( uint32_t i = 0; i < 3; i++ ) {
        ASSERT_EQ(kernel.at(i),kernel.at(4-i));
    }
    // peak should be at the center
    ASSERT_EQ(kernel.element_max(),kernel.at(2));
    ASSERT_TRUE(kernel.at(2)>kernel.at(1) && kernel.at(2)>kernel.at(3));

    kernel = FactoryKernel::gaussian1D<F32>(-1.0,4);
    ASSERT_FLOAT_EQ(1.0f,kernel.sum());
    for( uint32_t i = 0; i < 2; i++ ) {
        ASSERT_EQ(kernel.at(i),kernel.at(3-i));
    }
    // peak should be at the center
    ASSERT_EQ(kernel.element_max(),kernel.at(2));
    ASSERT_EQ(kernel.at(1),kernel.at(2));
    ASSERT_TRUE(kernel.at(3)<kernel.at(2) && kernel.at(1)>kernel.at(0));
}

TEST_F(FactoryKernelTest, computePDF) {
    ASSERT_NEAR(5.39909665131881e-02,FactoryKernel::computePDF(0, 1, -2),1e-8);
    ASSERT_NEAR(2.4197072451914342e-01,FactoryKernel::computePDF(0, 1, -1),1e-8);
    ASSERT_NEAR(2.4197072451914342e-01,FactoryKernel::computePDF(0, 1,  1),1e-8);
    ASSERT_NEAR(5.39909665131881e-02,FactoryKernel::computePDF(0, 1,  2),1e-8);

}

TEST_F(FactoryKernelTest, float_to_int) {
    Kernel1D<F64> kernel_F64(2,{0.1,0.3,0.5,0.1});

    Kernel1D<S32> kernel_S32 = FactoryKernel::float_to_int<S32>(kernel_F64,(F64)0.1);
    ASSERT_EQ(1,kernel_S32.at(0));
    ASSERT_EQ(2,kernel_S32.at(1)); // hmm should really be 3, but the java code doesn't round
    ASSERT_EQ(5,kernel_S32.at(2));
    ASSERT_EQ(1,kernel_S32.at(3));
}

/// Elements smaller than min_frac times the largest are ignored when choosing the divisor, like BoofCV
TEST_F(FactoryKernelTest, float_to_int_tiny_elements) {
    Kernel1D<F32> kernel(2,{0.001f,0.2f,0.5f,0.2f,0.001f});

    Kernel1D<S32> found = FactoryKernel::float_to_int<S32>(kernel,0.01f);
    std::vector<S32> expected = {0,1,2,1,0};
    for (uint32_t i = 0; i < kernel.width; i++)
        ASSERT_EQ(expected[i],found.at(i));
}

/// Integer Gaussian kernels from BoofCV's FactoryKernelGaussian.gaussian(Kernel1D_S32.class, sigma, radius)
TEST_F(FactoryKernelTest, gaussian1D_S32_same_as_BoofCV) {
    struct Case { double sigma; int32_t width; std::vector<S32> expected; };
    std::vector<Case> cases = {
            {-1, 3, {1,4,1}},
            {-1, 11, {1,2,5,8,11,13,11,8,5,2,1}},
            {1.5, -1, {1,4,14,28,35,28,14,4,1}},
            {2.0, -1, {1,3,7,13,20,22,20,13,7,3,1}},
            {0.7, 7, {0,1,21,59,21,1,0}},
            {3.0, -1, {1,2,3,6,9,12,14,15,14,12,9,6,3,2,1}}};
    for (const Case& c : cases) {
        Kernel1D<S32> found = FactoryKernel::gaussian1D<S32>(c.sigma,c.width);
        ASSERT_EQ(c.expected.size(),found.width);
        for (uint32_t i = 0; i < found.width; i++)
            ASSERT_EQ(c.expected[i],found.at(i));
    }
}


