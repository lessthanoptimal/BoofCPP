#include <stdio.h> // needed for jpeglib since it doesn't define everything it needed
#include <csetjmp> // needed on windows
#include <jpeglib.h>
#include <png.h>
#include <libpng16/png.h> // required for windows + cygwin


#include "benchmark_common.h"

using namespace boofcv;

struct my_jpeg_error {
    struct jpeg_error_mgr   base;
    jmp_buf                 env;
};

static void my_output_message(struct jpeg_common_struct *com)
{
    struct my_jpeg_error *err = (struct my_jpeg_error *)com->err;
    char buf[JMSG_LENGTH_MAX];

    err->base.format_message(com, buf);
    fprintf(stderr, "JPEG error: %s", buf);
}

static void my_error_exit(struct jpeg_common_struct *com)
{
    struct my_jpeg_error *err = (struct my_jpeg_error *)com->err;

    my_output_message(com);
    longjmp(err->env, 0);
}

static struct jpeg_error_mgr *my_error_mgr(struct my_jpeg_error *err)
{
    jpeg_std_error(&err->base);

    err->base.error_exit = my_error_exit;
    err->base.output_message = my_output_message;

    return &err->base;
}

bool boofcv::load_jpeg( const char *filename , Interleaved<U8>& dst) {
    FILE *infile = fopen(filename, "rb");
    struct jpeg_decompress_struct dinfo;
    struct my_jpeg_error err;
    int y;

    if (!infile) {
        perror("can't open input file");
        return false;
    }

    memset(&dinfo, 0, sizeof(dinfo));
    dinfo.err = my_error_mgr(&err);

    if (setjmp(err.env))
        goto fail;

    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, infile);

    jpeg_read_header(&dinfo, TRUE);
    dinfo.output_components = 1;
    dinfo.out_color_space = JCS_GRAYSCALE;
    jpeg_start_decompress(&dinfo);

    if (dinfo.output_components != 1) {
        fprintf(stderr, "Unexpected number of output components: %d",
                dinfo.output_components);
        goto fail;
    }

    dst.reshape(dinfo.output_width, dinfo.output_height,(uint32_t)dinfo.output_components);

    for (y = 0; y < dinfo.output_height; y++) {
        JSAMPROW row_pointer = &dst.data[dst.offset + dst.stride*y];

        jpeg_read_scanlines(&dinfo, &row_pointer, 1);
    }

    fclose(infile);
    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);
    return true;

    fail:
    fclose(infile);
    jpeg_destroy_decompress(&dinfo);
    return false;
}

/* hacked from https://dev.w3.org/Amaya/libpng/example.c
 *
 * Check if a file is a PNG image using png_sig_cmp(). Returns 1 if the given
 * file is a PNG and 0 otherwise.
 */
#define PNG_BYTES_TO_CHECK 4
int check_if_png(const char *filename)
{
    int ret = 0;
    FILE *infile = NULL;
    unsigned char buf[PNG_BYTES_TO_CHECK];

    /* Open the prospective PNG file. */
    if ((infile = fopen(filename, "rb")) == NULL)
        goto out;

    /* Read in some of the signature bytes */
    if (fread(buf, 1, PNG_BYTES_TO_CHECK, infile) != PNG_BYTES_TO_CHECK)
        goto out;

    /*
     * Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
     * png_sig_cmp() returns zero if the image is a PNG and nonzero if it
     * isn't a PNG.
     */
    if (png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK) == 0)
        ret = 1;

    /* FALLTHROUGH */
    out:
    if (infile)
        fclose(infile);
    return (ret);
}

bool boofcv::load_png(const char *filename , Interleaved<U8>& dst)
{
    int width, height, rowbytes, interlace_type, number_passes = 1;
    png_uint_32 trns;
    png_byte color_type, bit_depth;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    FILE *infile = NULL;
    uint8_t *image;
    int ret = -1;
    int pass;

    if ((infile = fopen(filename, "rb")) == NULL)
        goto out;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto out;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        goto out;

    if (setjmp(png_jmpbuf(png_ptr)))
        goto out;

    png_init_io(png_ptr, infile);

    png_read_info(png_ptr, info_ptr);

    color_type     = png_get_color_type(png_ptr, info_ptr);
    bit_depth      = png_get_bit_depth(png_ptr, info_ptr);
    interlace_type = png_get_interlace_type(png_ptr, info_ptr);

    // Read any color_type into 8bit depth, Grayscale format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    if ((trns = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)))
        png_set_tRNS_to_alpha(png_ptr);

    if (bit_depth == 16)
#if PNG_LIBPNG_VER >= 10504
        png_set_scale_16(png_ptr);
#else
        png_set_strip_16(png_ptr);
#endif

    if ((trns) || color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE ||
        color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);
    }

    if (interlace_type != PNG_INTERLACE_NONE)
        number_passes = png_set_interlace_handling(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    width    = png_get_image_width(png_ptr, info_ptr);
    height   = png_get_image_height(png_ptr, info_ptr);
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    if (rowbytes != width) {
        fprintf(stderr,
                "load_png: expected rowbytes to be %u but got %u\n",
                width, rowbytes);
        goto out;
    }

    // TODO determine the number of channels/bands
    dst.reshape( (uint32_t)width, (uint32_t)height, 3);

    for (pass = 0; pass < number_passes; pass++) {
        int y;

        for (y = 0; y < height; y++) {
            png_bytep row_pointer = &dst.data[dst.offset + y * dst.stride];
            png_read_rows(png_ptr, &row_pointer, NULL, 1);
        }
    }

    png_read_end(png_ptr, info_ptr);

    ret = 0;
    /* FALLTHROUGH */
    out:
    /* cleanup */
    if (png_ptr) {
        if (info_ptr)
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        else
            png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    }
    if (infile)
        fclose(infile);
    return ret == 0;
}
