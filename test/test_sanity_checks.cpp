#include "gtest/gtest.h"
#include "sanity_checks.h"

using namespace std;
using namespace boofcv;

// Test methods

TEST(IndependentMethod, checkSameShape2) {
    Gray<U8> imgA(10,20);
    Gray<U8> imgB(10,20);

    checkSameShape(imgA,imgB);

    try {
        Gray<U8> imgC(11,20);
        checkSameShape(imgA,imgC);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {

    }

    try {
        Gray<U8> imgC(10,21);
        checkSameShape(imgA,imgC);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {

    }
}

TEST(IndependentMethod, checkSameShape3) {
    Gray<U8> imgA(10,20);
    Gray<U8> imgB(10,20);
    Gray<U8> imgC(10,20);

    checkSameShape(imgA,imgB,imgC);

    try {
        Gray<U8> imgD(11,20);
        checkSameShape(imgA,imgB,imgD);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {}

    try {
        Gray<U8> imgD(10,21);
        checkSameShape(imgA,imgB,imgD);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {}

    try {
        Gray<U8> imgD(11,20);
        checkSameShape(imgA,imgD,imgC);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {}

    try {
        Gray<U8> imgD(10,21);
        checkSameShape(imgA,imgD,imgC);
        EXPECT_FALSE(true);
    } catch( invalid_argument &ignore ) {}
}
