#include "gtest/gtest.h"
#include "contour.h"
#include "image_misc_ops.h"

#include "print_structures.h"

using namespace std;
using namespace boofcv;

void add_border(const Gray<U8>& src , Gray<U8>& border ) {
    border.reshape(src.width+2,src.height+2);
    border.makeSubimage(1,1,border.width-1,border.height-1).setTo(src);
    ImageMiscOps::fill_border(border,(U8)0,1);
}

void checkAllContour( int blobX , int blobY , ConnectRule rule ) {
    ContourTracer tracer(rule);

    Gray<U8> input(4,5);
    Gray<S32> label(input.width,input.height);
    Gray<U8> border;

    PackedSet<Point2D<S32>> queue;

    // exhaustively try all initial locations
    for( uint32_t y = 0; y < input.height-blobY+1; y++ ) {
        for( uint32_t x = 0; x < input.width -blobX + 1; x++) {
            ImageMiscOps::fill(input,(U8)0);

            // fill in a solid image which is marked as all contour
            Gray<U8> inner = input.makeSubimage(x,y,blobX+x,blobY+y);
            ImageMiscOps::fill(inner,(U8)1);

            add_border(input,border);

            // process the image
            ImageMiscOps::fill(label,(S32)0);
            queue.reset();
            tracer.set_inputs(border,label,queue);
            tracer.trace(2,x+1,y+1,true);

            // forward then back
            ASSERT_EQ(blobX*blobY,queue.size_of_tail());

            // see if the image has been correctly labeled
            for( uint32_t yy = 0; yy < input.height; yy++ ) {
                for( uint32_t xx = 0; xx < input.width; xx++ ) {
                    bool isOne = false;
                    if( x <= xx && (xx-x) < blobX && y <= yy && (yy-y) < blobY  ) {
                        isOne = true;
                    }
                    if( isOne )
                        ASSERT_EQ(2,label.at(xx,yy));
                    else
                        ASSERT_EQ(0,label.at(xx,yy));
                }
            }
        }
    }
}


TEST(PackedSet, constructor_4) {
    ContourTracer tracer(ConnectRule::FOUR);

    ASSERT_EQ(4,tracer.ruleN);
    // should check next direction. need to reread the paper to see why it is the way it is
}

TEST(PackedSet, constructor_8) {
    ContourTracer tracer(ConnectRule::EIGHT);

    ASSERT_EQ(8,tracer.ruleN);
    // should check next direction. need to reread the paper to see why it is the way it is
}

TEST(PackedSet, single_pixel) {
    checkAllContour(1,1,ConnectRule::FOUR);
    checkAllContour(1,1,ConnectRule::EIGHT);
}


TEST(PackedSet, line_2) {
    checkAllContour(2,1,ConnectRule::FOUR);
    checkAllContour(1,2,ConnectRule::FOUR);
    checkAllContour(2,1,ConnectRule::EIGHT);
    checkAllContour(1,2,ConnectRule::EIGHT);
}

TEST(PackedSet, square_2) {
    checkAllContour(2,2,ConnectRule::FOUR);
    checkAllContour(2,2,ConnectRule::EIGHT);
}


TEST(PackedSet, funky1) {

}

TEST(PackedSet, funky2) {

}
TEST(PackedSet, funky3) {

}

TEST(PackedSet, funky4) {

}

TEST(PackedSet, interior1) {

}

TEST(PackedSet, interior2) {

}

/**
 * Make sure it is marking surrounding white pixels
 */
TEST(PackedSet, mark_white) {

}