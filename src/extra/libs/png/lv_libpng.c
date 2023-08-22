/**
 * @file lv_libpng.c
 * @author shezw
 *
 * Now available on Linux/MacOS/Windows
 * Not test on RTOS
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#if LV_USE_LIBPNG

#include "lv_png.h"
#include "lodepng.h"
#include <stdlib.h>

/**
 * USE libpng16
 */
#include <png.h>
#include <zlib.h>
typedef struct {
    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep * row_pointers;
    void * buf;
} png_img_t;



static uint8_t * libpng_read_image(png_img_t * p, const char* file_name )
{
    char header[8];    // 8 is the maximum size that can be checked

    /*open file and test for it being a png*/
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        printf("%s", "PNG file %s could not be opened for reading");
        return 0;
    }

    size_t rcnt = fread(header, 1, 8, fp);
    if (rcnt != 8 || png_sig_cmp((png_const_bytep)header, 0, 8)) {
        printf("%s is not recognized as a PNG file", file_name);
        return 0;
    }

    /*initialize stuff*/
    p->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!p->png_ptr) {
        printf("%s", "png_create_read_struct failed");
        return 0;
    }

    p->info_ptr = png_create_info_struct(p->png_ptr);
    if (!p->info_ptr) {
        printf("%s", "png_create_info_struct failed");
        return 0;
    }
    if (setjmp(png_jmpbuf(p->png_ptr))) {
        printf("%s", "Error during init_io");
        return 0;
    }
    png_init_io(p->png_ptr, fp);
    png_set_sig_bytes(p->png_ptr, 8);

    png_read_info(p->png_ptr, p->info_ptr);

    p->width =      png_get_image_width(p->png_ptr,  p->info_ptr);
    p->height =     png_get_image_height(p->png_ptr, p->info_ptr);
    p->color_type = png_get_color_type(p->png_ptr,   p->info_ptr);
    p->bit_depth =  png_get_bit_depth(p->png_ptr,    p->info_ptr);

    p->number_of_passes = png_set_interlace_handling(p->png_ptr);

    png_set_bgr(p->png_ptr);

    if( p->color_type==PNG_COLOR_TYPE_PALETTE )
        png_set_palette_to_rgb(p->png_ptr);

    if( p->color_type==PNG_COLOR_TYPE_RGB ) // add alpha channel
        png_set_add_alpha( p->png_ptr, LV_OPA_MAX, PNG_FILLER_AFTER );

    png_read_update_info(p->png_ptr, p->info_ptr);

    /*read file*/
    if (setjmp(png_jmpbuf(p->png_ptr))) {
        printf("%s", "Error during read_image");
        return 0;
    }
    p->row_pointers = (png_bytep*) lv_mem_alloc(sizeof(png_bytep) * p->height);

    p->buf = (uint8_t*) lv_mem_alloc( p->height * p->width * 4 );

    for (int y=0; y<p->height; y++)
        p->row_pointers[y] = p->buf + (p->width*4*y);

    png_read_image(p->png_ptr, p->row_pointers);

    fclose(fp);
    return p->buf;
}

static void libpng_release(png_img_t * p)
{
    lv_mem_free(p->row_pointers);

    png_destroy_read_struct(&p->png_ptr, &p->info_ptr, NULL);
}


static uint8_t * libpng_read_data( const char * path, int * width, int * height ) {

    const char * real_path = lv_fs_get_real_path(path);

    char * file = lv_mem_alloc( strlen(path)+1 );
    memset( file, 0, strlen(path)+1 );

    png_img_t * p = lv_mem_alloc( sizeof(png_img_t) );
    memset(p,0,sizeof(png_img_t));

    uint8_t * img_data = libpng_read_image( p, real_path );

    *width = p->width;
    *height= p->height;

    libpng_release(p);
    lv_mem_free(p);

    return img_data;
}


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(struct _lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header);
static lv_res_t decoder_open(lv_img_decoder_t * dec, lv_img_decoder_dsc_t * dsc);
static void decoder_close(lv_img_decoder_t * dec, lv_img_decoder_dsc_t * dsc);
static void convert_color_depth(uint8_t * img, uint32_t px_cnt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Register the PNG decoder functions in LVGL
 */
void lv_png_init(void)
{
    lv_img_decoder_t * dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Get info about a PNG image
 * @param src can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
static lv_res_t decoder_info(struct _lv_img_decoder_t * decoder, const void * src, lv_img_header_t * header)
{
    (void) decoder; /*Unused*/
    lv_img_src_t src_type = lv_img_src_get_type(src);          /*Get the source type*/

    /*If it's a PNG file...*/
    if(src_type == LV_IMG_SRC_FILE) {
        const char * fn = src;

        if(strcmp(lv_fs_get_ext(fn), "png") == 0) {              /*Check the extension*/

            /* Read the width and height from the file. They have a constant location:
             * [16..23]: width
             * [24..27]: height
             */
            uint32_t size[2];
            lv_fs_file_t f;
            lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);
            if(res != LV_FS_RES_OK) return LV_RES_INV;

            lv_fs_seek(&f, 16, LV_FS_SEEK_SET);

            uint32_t rn;
            lv_fs_read(&f, &size, 8, &rn);
            lv_fs_close(&f);

            if(rn != 8) return LV_RES_INV;

            /*Save the data in the header*/
            header->always_zero = 0;
            header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
            /*The width and height are stored in Big endian format so convert them to little endian*/
            header->w = (lv_coord_t)((size[0] & 0xff000000) >> 24) + ((size[0] & 0x00ff0000) >> 8);
            header->h = (lv_coord_t)((size[1] & 0xff000000) >> 24) + ((size[1] & 0x00ff0000) >> 8);

            return LV_RES_OK;
        }
    }
    /*If it's a PNG file in a  C array...*/
    else if(src_type == LV_IMG_SRC_VARIABLE) {
        const lv_img_dsc_t * img_dsc = src;
        const uint32_t data_size = img_dsc->data_size;
        const uint8_t magic[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
        if(data_size < sizeof(magic)) return LV_RES_INV;
        if(memcmp(magic, img_dsc->data, sizeof(magic))) return LV_RES_INV;
        header->always_zero = 0;
        header->cf = img_dsc->header.cf;       /*Save the color format*/
        header->w = img_dsc->header.w;         /*Save the color width*/
        header->h = img_dsc->header.h;         /*Save the color height*/
        return LV_RES_OK;
    }

    return LV_RES_INV;         /*If didn't succeeded earlier then it's an error*/
}

/**
 * Open a PNG image and return the decided image
 * @param src can be file name or pointer to a C array
 * @param style style of the image object (unused now but certain formats might use it)
 * @return pointer to the decoded image or `LV_IMG_DECODER_OPEN_FAIL` if failed
 */
static lv_res_t decoder_open(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    (void) decoder; /*Unused*/
    uint32_t error;                 /*For the return values of PNG decoder functions*/

    uint8_t * img_data = NULL;

    /*If it's a PNG file...*/
    if(dsc->src_type == LV_IMG_SRC_FILE) {
        const char * fn = dsc->src;
        if(strcmp(lv_fs_get_ext(fn), "png") == 0) {              /*Check the extension*/

            int width =0, height = 0; // +-
            img_data = libpng_read_data( fn, &width, &height ); // +-
            dsc->img_data = img_data; // +-
            return LV_RES_OK;     /*The image is fully decoded. Return with its pointer*/
        }
    }
    /*If it's a PNG file in a  C array...*/
    else if(dsc->src_type == LV_IMG_SRC_VARIABLE) {
        const lv_img_dsc_t * img_dsc = dsc->src;
        uint32_t png_width;             /*No used, just required by he decoder*/
        uint32_t png_height;            /*No used, just required by he decoder*/

        /*Decode the image in ARGB8888 */
        error = lodepng_decode32(&img_data, &png_width, &png_height, img_dsc->data, img_dsc->data_size);

        if(error) {
            if(img_data != NULL) {
                lv_mem_free(img_data);
            }
            return LV_RES_INV;
        }

        /*Convert the image to the system's color depth*/
        convert_color_depth(img_data,  png_width * png_height);

        dsc->img_data = img_data;
        return LV_RES_OK;     /*Return with its pointer*/
    }

//     return LV_RES_INV;    /*If not returned earlier then it failed*/
}

/**
 * Free the allocated resources
 */
static void decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    LV_UNUSED(decoder); /*Unused*/
    if(dsc->img_data) {
        lv_mem_free((uint8_t *)dsc->img_data);
        dsc->img_data = NULL;
    }
}

/**
 * If the display is not in 32 bit format (ARGB888) then covert the image to the current color depth
 * @param img the ARGB888 image
 * @param px_cnt number of pixels in `img`
 */
static void convert_color_depth(uint8_t * img, uint32_t px_cnt)
{
#if LV_COLOR_DEPTH == 32
    lv_color32_t * img_argb = (lv_color32_t *)img;
    lv_color_t c;
    lv_color_t * img_c = (lv_color_t *) img;
    uint32_t i;
    for(i = 0; i < px_cnt; i++) {
        c = lv_color_make(img_argb[i].ch.red, img_argb[i].ch.green, img_argb[i].ch.blue);
        img_c[i].ch.red = c.ch.blue;
        img_c[i].ch.blue = c.ch.red;
    }
#elif LV_COLOR_DEPTH == 16
    lv_color32_t * img_argb = (lv_color32_t *)img;
    lv_color_t c;
    uint32_t i;
    for(i = 0; i < px_cnt; i++) {
        c = lv_color_make(img_argb[i].ch.blue, img_argb[i].ch.green, img_argb[i].ch.red);
        img[i * 3 + 2] = img_argb[i].ch.alpha;
        img[i * 3 + 1] = c.full >> 8;
        img[i * 3 + 0] = c.full & 0xFF;
    }
#elif LV_COLOR_DEPTH == 8
    lv_color32_t * img_argb = (lv_color32_t *)img;
    lv_color_t c;
    uint32_t i;
    for(i = 0; i < px_cnt; i++) {
        c = lv_color_make(img_argb[i].ch.red, img_argb[i].ch.green, img_argb[i].ch.blue);
        img[i * 2 + 1] = img_argb[i].ch.alpha;
        img[i * 2 + 0] = c.full;
    }
#elif LV_COLOR_DEPTH == 1
    lv_color32_t * img_argb = (lv_color32_t *)img;
    uint8_t b;
    uint32_t i;
    for(i = 0; i < px_cnt; i++) {
        b = img_argb[i].ch.red | img_argb[i].ch.green | img_argb[i].ch.blue;
        img[i * 2 + 1] = img_argb[i].ch.alpha;
        img[i * 2 + 0] = b > 128 ? 1 : 0;
    }
#endif
}



#endif
