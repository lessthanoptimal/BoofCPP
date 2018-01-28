#include <exception>
#include <stdexcept>

#include "sanity_checks.h"

using namespace boofcv;
using namespace std;

void checkSameShape( const ImageBase& a , const ImageBase& b ) {
    if( a.width != b.width || a.height != b.height )
        throw invalid_argument("Image shapes do not match");
}