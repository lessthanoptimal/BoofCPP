#include "gtest/gtest.h"
#include "contour.h"
#include "image_misc_ops.h"
#include "image_statistics.h"
#include "image_border.h"

#include "print_structures.h"

using namespace std;
using namespace boofcv;

void add_border(const Gray<U8>& src , Gray<U8>& border ) {
    border.reshape(src.width+2,src.height+2);
    border.makeSubimage(1,1,border.width-1,border.height-1).copy(src);
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
            inner.copy(pattern);

            add_border(input,border);

            // process the image
            ImageMiscOps::fill(label,(S32)0);
            queue.clear();
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

class TestLinearContourLabelChang2004 {
public:
    Gray<U8> TEST1;
    Gray<U8> TEST2;
    Gray<U8> TEST3;
    Gray<U8> TEST4;

    std::vector<Point2D<S32>> local;

    TestLinearContourLabelChang2004() {
        TEST1 = Gray<U8>({{0,0,0,0,0,0,0,1,0,0,0,1,1},
                          {0,0,0,0,0,0,0,1,0,0,0,1,1},
                          {0,0,0,0,0,0,0,1,0,0,1,1,0},
                          {0,0,0,0,0,0,0,0,1,1,1,1,0},
                          {0,0,1,0,0,0,0,0,1,1,1,0,0},
                          {0,0,1,0,0,0,1,1,1,1,1,0,0},
                          {1,1,1,1,1,1,1,1,1,1,0,0,0},
                          {0,0,0,1,1,1,1,1,0,0,0,0,0}});

        TEST2 = Gray<U8>({{0,0,1,0,0,0,0,1,0,0,0,0,0},
                          {0,1,0,1,0,0,1,0,0,1,0,0,0},
                          {0,0,1,0,0,1,0,1,0,1,1,1,0},
                          {0,0,0,0,1,0,0,0,1,1,1,1,0},
                          {0,0,1,0,1,0,0,0,1,0,0,0,0},
                          {0,0,0,0,1,0,1,1,1,0,1,1,0},
                          {1,1,1,0,0,1,0,0,1,0,0,1,0},
                          {0,0,0,1,1,1,1,1,0,0,0,0,0}});

        TEST3 = Gray<U8>({{0,0,0,0,0},
                          {0,1,1,1,0},
                          {0,1,1,1,0},
                          {0,1,0,1,0},
                          {0,1,1,1,0},
                          {0,0,1,0,0},
                          {0,0,0,0,0}});

        TEST4 = Gray<U8>({{0,0,0,0,0,0,0},
                          {0,0,1,1,1,1,1},
                          {0,1,0,1,1,1,1},
                          {0,1,1,1,0,1,1},
                          {0,1,1,1,1,1,1},
                          {0,1,1,1,1,1,1},
                          {0,1,1,1,1,1,1},
                          {0,0,0,0,0,0,0}});

        local.push_back(Point2D<S32>(-1,-1));
        local.push_back(Point2D<S32>( 0,-1));
        local.push_back(Point2D<S32>( 1,-1));
        local.push_back(Point2D<S32>( 1, 0));
        local.push_back(Point2D<S32>( 1, 1));
        local.push_back(Point2D<S32>( 0, 1));
        local.push_back(Point2D<S32>(-1, 1));
        local.push_back(Point2D<S32>(-1, 0));
        local.push_back(Point2D<S32>(-1, -1));
    }

    /**
     * Create an unordered list of all points in the internal and external contour
     */
    vector<Point2D<S32>> findContour8(Gray<S32>& labeled, S32 target) {
        vector<Point2D<S32>> list;

        ImageBorderValue<S32> border(labeled,(S32)0);

        for( uint32_t y = 0; y < labeled.height; y++ ) {
            for( uint32_t x = 0; x < labeled.width; x++ ) {
                if( target == labeled.at(x,y) ) {

                    bool isContour = false;
                    for( uint32_t i = 0; i < local.size()-1; i++ ) {
                        Point2D<S32>& a = local.at(i);
                        Point2D<S32>& b = local.at(i+1);

                        if( border.get( x + a.x, y + a.y) != target && border.get(x + b.x, y + b.y) != target ) {
                            isContour = true;
                            break;
                        }
                    }

                    if( !isContour && border.get( x + 1, y) != target)
                        isContour = true;
                    if( !isContour && border.get( x - 1, y) != target)
                        isContour = true;
                    if( !isContour && border.get( x, y + 1) != target)
                        isContour = true;
                    if( !isContour && border.get( x, y - 1) != target)
                        isContour = true;

                    if( isContour )
                        list.push_back( Point2D<S32> (x,y) );
                }
            }
        }
        return list;
    }

    std::vector<Point2D<S32>> findContour4(Gray<S32>& labeled, S32 target) {
        vector<Point2D<S32>> list;

        ImageBorderValue<S32> border(labeled,(S32)0);

        for( uint32_t y = 0; y < labeled.height; y++ ) {
            for( uint32_t x = 0; x < labeled.width; x++ ) {
                if( target == labeled.at(x,y) ) {

                    bool isContour = false;
                    for( uint32_t i = 0; i < local.size(); i++ ) {
                        Point2D<S32>& a = local.at(i);
                        if( border.get(x + a.x, y + a.y) != target ) {
                            isContour = true;
                        }
                    }

                    if( isContour )
                        list.push_back( Point2D<S32>(x,y) );
                }
            }
        }
        return list;
    }

    void addPointsToList( LinearContourLabelChang2004& alg , uint32_t set , std::vector<Point2D<S32>>& list ) {
        uint32_t length = alg.packedPoints.set_info.at(set).size;

        for( uint32_t i = 0; i < length; i++ ) {
            list.push_back( alg.packedPoints.at(set,i));
        }
    }

    vector<Point2D<S32>> removeDuplicates( vector<Point2D<S32>>& list ) {
        vector<Point2D<S32>> ret;

        for( uint32_t i = 0; i < list.size(); i++ ) {
            Point2D<S32> &p = list.at(i);
            bool matched = false;
            for( uint32_t j = i+1; j < list.size(); j++ ) {
                Point2D<S32> &c = list.at(j);
                if( p.x == c.x && p.y == c.y ) {
                    matched = true;
                    break;
                }
            }
            if( !matched ) {
                ret.push_back(p);
            }
        }
        return ret;
    }

    /**
     * Creates a list of every pixel with the specified label that is on the contour.  Removes duplicate points
     * in the found contour.  Sees if the two lists are equivalent.
     *
     * @param rule Which connectivity rule is being tested
     */
    void checkContour(LinearContourLabelChang2004& alg, Gray<S32>& labeled , ConnectRule rule ) {

        std::vector<ContourPacked>&contours = alg.contours;

        for( size_t i = 0; i < contours.size(); i++ ) {
//			System.out.println("=================== Contour "+i);
            ContourPacked& c = contours.at(i);

            ASSERT_TRUE(c.id > 0 );

            std::vector<Point2D<S32>> found;
            addPointsToList(alg,c.externalIndex,found);
            for(uint32_t j = 0; j < c.internalIndexes.size(); j++ ) {
                addPointsToList(alg,c.internalIndexes.at(j),found);
            }

            // there can be duplicate points, remove them
            found = removeDuplicates(found);

            // see if the two lists are equivalent
            std::vector<Point2D<S32>> expected = rule == ConnectRule::EIGHT ? findContour8(labeled, c.id) : findContour4(labeled, c.id);

//			labeled.print();
//			System.out.println("------------------");
//			print(found,labeled.width,labeled.height);
//			print(expected,labeled.width,labeled.height);

            ASSERT_EQ(expected.size(),found.size());

            for( Point2D<S32> f : found ) {
                bool match = false;
                for( uint32_t j = 0; j < expected.size(); j++ ) {
                    Point2D<S32> &e = expected.at(j);
                    if( f.x == e.x && f.y == e.y ) {
                        match = true;
                        break;
                    }
                }
                ASSERT_TRUE(match);
            }
        }
    }
};


TEST(LinearContourLabelChang2004, test1_4) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST1;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::FOUR);
    alg.process(input, labeled);

    ASSERT_EQ(2, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::FOUR);
}

TEST(LinearContourLabelChang2004, test1_8) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST1;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::EIGHT);
    alg.process(input, labeled);

    ASSERT_EQ(1, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::EIGHT);
}

TEST(LinearContourLabelChang2004, test2_4) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST2;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::FOUR);
    alg.process(input, labeled);

    ASSERT_EQ(14, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::FOUR);
}

TEST(LinearContourLabelChang2004, test2_8) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST2;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::EIGHT);
    alg.process(input, labeled);

    ASSERT_EQ(4, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::EIGHT);
}

TEST(LinearContourLabelChang2004, test3_4) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST4;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::FOUR);
    alg.process(input, labeled);

    ASSERT_EQ(1, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::FOUR);
}

TEST(LinearContourLabelChang2004, test3_8) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST4;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::EIGHT);
    alg.process(input, labeled);

    ASSERT_EQ(1, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::EIGHT);
}

TEST(LinearContourLabelChang2004, checkInnerOuterContour) {
    TestLinearContourLabelChang2004 testing;

    Gray<U8> &input = testing.TEST3;

    Gray<S32> labeled(input.width,input.height);
    LinearContourLabelChang2004 alg(ConnectRule::EIGHT);
    alg.process(input, labeled);

    ASSERT_EQ(1, alg.contours.size());
    testing.checkContour(alg, labeled, ConnectRule::EIGHT);

    ContourPacked& c = alg.contours.at(0);
    ASSERT_EQ(10, alg.packedPoints.set_info.at(c.externalIndex).size);
    ASSERT_EQ(1,  c.internalIndexes.size());
    ASSERT_EQ(4,  alg.packedPoints.set_info.at(c.externalIndex+1).size);
}