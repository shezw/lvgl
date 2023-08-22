//
// Created by shezw on 2023/8/22.
//

#ifndef LVGL_LV_LIBPNG_H
#define LVGL_LV_LIBPNG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lv_conf_internal.h"
#if LV_USE_LIBPNG

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register the PNG decoder functions in LVGL
 */
void lv_png_init(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
}
#endif

#endif //LV_USE_LIBPNG

#endif //LVGL_LV_LIBPNG_H
