#include <boofcv/image_types.h>
#include <boofcv/base_types.h>

namespace boofcv {
    bool load_jpeg( const char *filename , Interleaved<U8>& dst );
    bool load_png( const char *filename ,  Interleaved<U8>& dst );
}