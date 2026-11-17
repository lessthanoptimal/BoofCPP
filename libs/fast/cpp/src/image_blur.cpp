#include "boofcv/fast/image_blur.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace boofcv::fast {
    namespace {
        /// Number of pixels inside the image in the window [i - radius, i + radius]
        inline uint32_t window_count( uint32_t i, uint32_t radius, uint32_t length ) {
            uint32_t lower = i > radius ? i - radius : 0;
            uint32_t upper = std::min(i + radius, length - 1);
            return upper - lower + 1;
        }

        /// Horizontal mean of R rows of column sums at once, so that their running sums are independent of each other.
        /// Each row of 'sums' is padded with radius + 1 zeros in front and radius zeros after
        template<int R, class E>
        void horizontal_rows( E* const sums[], const E* inv_count_x, const E inv_count_y[], E* const out[],
                              uint32_t width, uint32_t radius ) {
            const uint32_t length = 2*radius + 1;
            E total[R];
            for (int r = 0; r < R; r++) {
                total[r] = 0;
                for (uint32_t i = 0; i < length; i++)
                    total[r] += sums[r][i];
            }
            for (uint32_t x = 0; x < width; x++) {
                for (int r = 0; r < R; r++) {
                    total[r] += sums[r][x + length] - sums[r][x];
                    out[r][x] = total[r]*inv_count_x[x]*inv_count_y[r];
                }
            }
        }

        /// Fused mean: column sums over the rows inside the vertical window are updated one row at a time, then
        /// summed along x. If input and output share their pixels, the input is copied into 'storage' first
        template<class E>
        void mean_impl( const Gray<E>& original, Gray<E>& output, uint32_t radius_x, uint32_t radius_y,
                        Gray<E>& storage ) {
            if (radius_x == 0 || radius_y == 0)
                throw std::invalid_argument("Radius must be > 0");
            const Gray<E>* source = &original;
            if (original.data == output.data) {
                storage.copy(original);
                source = &storage;
            }
            const Gray<E>& input = *source;
            output.reshape(input.width, input.height);
            const uint32_t width = input.width, height = input.height;
            if (width == 0 || height == 0)
                return;
            // A window larger than the image covers the same pixels
            radius_x = std::min(radius_x, width);
            radius_y = std::min(radius_y, height);

            constexpr int R = 4;
            const uint32_t padded = width + 2*radius_x + 1;
            const uint32_t front = radius_x + 1;
            std::vector<E> sums(R*padded, 0);
            std::vector<E> inv_count_x(width);
            for (uint32_t x = 0; x < width; x++)
                inv_count_x[x] = (E)1/(E)window_count(x, radius_x, width);

            // Each row of 'sums' is updated from the row before it. The last row starts as the column sums of the
            // rows in the window of the row before the first one
            E* previous = sums.data() + (R - 1)*padded + front;
            for (uint32_t y = 0; y < std::min(radius_y, height); y++) {
                const E* row = input.data + input.offset + y*input.stride;
                for (uint32_t x = 0; x < width; x++)
                    previous[x] += row[x];
            }

            for (uint32_t y0 = 0; y0 < height; y0 += R) {
                const int rows = (int)std::min<uint32_t>(R, height - y0);
                E* sum_rows[R];
                E* out_rows[R];
                E inv_count_y[R];
                for (int r = 0; r < rows; r++) {
                    const uint32_t y = y0 + r;
                    E* column = sums.data() + r*padded + front;
                    const E* row_add = nullptr;
                    const E* row_remove = nullptr;
                    if (y + radius_y < height)
                        row_add = input.data + input.offset + (y + radius_y)*input.stride;
                    if (y > radius_y)
                        row_remove = input.data + input.offset + (y - radius_y - 1)*input.stride;
                    if (row_add && row_remove) {
                        for (uint32_t x = 0; x < width; x++)
                            column[x] = previous[x] + (row_add[x] - row_remove[x]);
                    } else if (row_add) {
                        for (uint32_t x = 0; x < width; x++)
                            column[x] = previous[x] + row_add[x];
                    } else if (row_remove) {
                        for (uint32_t x = 0; x < width; x++)
                            column[x] = previous[x] - row_remove[x];
                    } else if (column != previous) {
                        std::copy(previous, previous + width, column);
                    }
                    previous = column;
                    sum_rows[r] = column - front;
                    out_rows[r] = output.data + output.offset + y*output.stride;
                    inv_count_y[r] = (E)1/(E)window_count(y, radius_y, height);
                }
                if (rows == R) {
                    horizontal_rows<R>(sum_rows, inv_count_x.data(), inv_count_y, out_rows, width, radius_x);
                } else {
                    for (int r = 0; r < rows; r++)
                        horizontal_rows<1>(sum_rows + r, inv_count_x.data(), inv_count_y + r, out_rows + r,
                                           width, radius_x);
                }
            }
        }
    }

    void BlurImageOps::mean( const Gray<F32>& input, Gray<F32>& output, uint32_t radius, Gray<F32>& storage ) {
        mean_impl(input, output, radius, radius, storage);
    }

    void BlurImageOps::mean( const Gray<F64>& input, Gray<F64>& output, uint32_t radius, Gray<F64>& storage ) {
        mean_impl(input, output, radius, radius, storage);
    }

    void BlurImageOps::mean( const Gray<F32>& input, Gray<F32>& output, uint32_t radius_x, uint32_t radius_y,
                             Gray<F32>& storage ) {
        mean_impl(input, output, radius_x, radius_y, storage);
    }

    void BlurImageOps::mean( const Gray<F64>& input, Gray<F64>& output, uint32_t radius_x, uint32_t radius_y,
                             Gray<F64>& storage ) {
        mean_impl(input, output, radius_x, radius_y, storage);
    }
}
