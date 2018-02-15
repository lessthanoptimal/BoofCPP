#include "gtest/gtest.h"
#include "contour.h"
#include "image_misc_ops.h"
#include "image_statistics.h"

#include "print_structures.h"

using namespace std;
using namespace boofcv;

void add_border(const Gray<U8>& src , Gray<U8>& border ) {
    border.reshape(src.width+2,src.height+2);
    border.makeSubimage(1,1,border.width-1,border.height-1).setTo(src);
    ImageMiscOps::fill_border(border,(U8)0,1);
}

void checkAllContour( const Gray<U8>& pattern , uint32_t expected_total,  ConnectRule rule ) {
    ContourTracer tracer(rule);

    Gray<U8> input(4,5);
    Gray<S32> label(input.width,input.height);
    Gray<U8> border;

    PackedSet<Point2D<S32>> queue;

    // exhaustively try all initial locations
    for( uint32_t y = 0; y < input.height - pattern.height+1; y++ ) {
        for( uint32_t x = 0; x < input.width - pattern.width + 1; x++) {
            ImageMiscOps::fill(input,(U8)0);

            // fill in a solid image which is marked as all contour
            Gray<U8> inner = input.makeSubimage(x,y,pattern.width+x,pattern.height+y);
            inner.setTo(pattern);

            add_border(input,border);

            // process the image
            ImageMiscOps::fill(label,(S32)0);
            queue.reset();
            tracer.set_inputs(border,label,queue);
            tracer.trace(2,x+1,y+1,true);

            // forward then back
            ASSERT_EQ(expected_total,queue.size_of_tail());

//            print(input);
//            print(label);

            // see if the image has been correctly labeled
            for( uint32_t yy = 0; yy < input.height; yy++ ) {
                for( uint32_t xx = 0; xx < input.width; xx++ ) {
                    bool isOne = false;
                    if( pattern.isInBounds(xx-x,yy-y)  ) {
                        isOne = pattern.at(xx-x,yy-y) == 1;
                    }
//                    printf("one %d at %d %d\n",isOne,xx,yy);
                    if( isOne )
                        ASSERT_EQ(2,label.at(xx,yy));
                    else
                        ASSERT_EQ(0,label.at(xx,yy));
                }
            }
        }
    }
}

void checkAllContour( const Gray<U8>& pattern , ConnectRule rule ) {
    uint32_t expected_total = ImageStatistics::sum(pattern);
    checkAllContour(pattern,expected_total,rule);
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
    Gray<U8> pattern({{1}});
    checkAllContour(pattern,ConnectRule::FOUR);
    checkAllContour(pattern,ConnectRule::EIGHT);
}


TEST(PackedSet, line_2) {
    Gray<U8> patternH({{1,1}});
    checkAllContour(patternH,ConnectRule::FOUR);
    checkAllContour(patternH,ConnectRule::EIGHT);

    Gray<U8> patternV({{1},{1}});
    checkAllContour(patternV,ConnectRule::FOUR);
    checkAllContour(patternV,ConnectRule::EIGHT);
}

TEST(PackedSet, square_2) {
    Gray<U8> pattern({{1,1},{1,1}});
    checkAllContour(pattern,ConnectRule::FOUR);
    checkAllContour(pattern,ConnectRule::EIGHT);
}


TEST(PackedSet, funky1) {
    Gray<U8> pattern({{1,0},
                      {0,1},
                      {1,0}});
    checkAllContour(pattern,4,ConnectRule::EIGHT);
}

TEST(PackedSet, funky2) {
    Gray<U8> pattern({{1,0,0,0},
                      {0,1,1,0},
                      {1,0,0,1}});
    checkAllContour(pattern,8,ConnectRule::EIGHT);
}

TEST(PackedSet, funky3) {
    Gray<U8> pattern({{0,1,0,0},
                      {0,1,1,0},
                      {1,1,0,1}});

    Gray<S32> labeled(pattern.width,pattern.height);
    Gray<U8> border;
    add_border(pattern,border);
    PackedSet<Point2D<S32>> queue;

    ContourTracer tracer(ConnectRule::EIGHT);

    tracer.set_inputs(border,labeled,queue);
    tracer.trace(2,1+1,0+1,true);

    ASSERT_EQ(7, queue._total_element);
    ASSERT_EQ(7, queue.size_of_tail());

}

TEST(PackedSet, funky4) {
    Gray<U8> pattern({{1,0,1},
                      {1,1,1},
                      {1,0,1}});
    checkAllContour(pattern,12,ConnectRule::FOUR);
}

TEST(PackedSet, interior1) {
    Gray<U8> pattern({{0,1,1,1,0},
                      {0,1,1,0,1},
                      {1,1,1,1,0}});

    Gray<S32> labeled(pattern.width,pattern.height);
    Gray<U8> border;
    add_border(pattern,border);
    PackedSet<Point2D<S32>> queue;

    ContourTracer tracer(ConnectRule::EIGHT);

    tracer.set_inputs(border,labeled,queue);
    tracer.trace(2,3+1,0+1,false);

    ASSERT_EQ(4, queue.size_of_tail());
}

TEST(PackedSet, interior2) {
    Gray<U8> pattern({{0,1,1,1,1},
                      {0,1,1,0,1},
                      {1,1,1,1,1}});

    Gray<S32> labeled(pattern.width,pattern.height);
    Gray<U8> border;
    add_border(pattern,border);
    PackedSet<Point2D<S32>> queue;

    ContourTracer tracer(ConnectRule::FOUR);

    tracer.set_inputs(border,labeled,queue);
    tracer.trace(2,3+1,0+1,false);

    ASSERT_EQ(8, queue.size_of_tail());
}

/**
 * Make sure it is marking surrounding white pixels
 */
TEST(PackedSet, mark_white) {
    Gray<U8> before({{0,0,0,0,0,0},
                     {0,0,1,1,0,0},
                     {0,0,0,1,0,0},
                     {0,0,0,0,0,0}});

    Gray<U8> after({{0,2,2,2,2,0},
                    {0,2,1,1,2,0},
                    {0,2,2,1,2,0},
                    {0,0,2,2,2,0}});

    Gray<S32> label(before.width,before.height);
    PackedSet<Point2D<S32>> queue;

    ContourTracer alg(ConnectRule::EIGHT);

    // process the image
    alg.set_inputs(before,label,queue);
    alg.trace(2,2,1,true);

    for( uint32_t i = 0; i < before.height; i++ ) {
        for( uint32_t j = 0; j < before.width; j++ ) {
            if( after.at(j,i) == 2 )
                ASSERT_EQ(255,before.at(j,i));
            else
                ASSERT_EQ(after.at(j,i),before.at(j,i));
        }
    }
}