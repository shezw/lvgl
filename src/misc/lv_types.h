/**
 * @file lv_types.h
 *
 */

#ifndef LV_TYPES_H
#define LV_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

// If __UINTPTR_MAX__ or UINTPTR_MAX are available, use them to determine arch size
#if defined(__UINTPTR_MAX__) && __UINTPTR_MAX__ > 0xFFFFFFFF
#define LV_ARCH_64

#elif defined(UINTPTR_MAX) && UINTPTR_MAX > 0xFFFFFFFF
#define LV_ARCH_64

// Otherwise use compiler-dependent means to determine arch size
#elif defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined (__aarch64__)
#define LV_ARCH_64

#endif

#define LV_OS_NONE          0
#define LV_OS_PTHREAD       1
#define LV_OS_FREERTOS      2
#define LV_OS_CMSIS_RTOS2   3
#define LV_OS_CUSTOM        255


#define LV_STDLIB_BUILTIN           0
#define LV_STDLIB_CLIB              1
#define LV_STDLIB_MICROPYTHON       2
#define LV_STDLIB_CUSTOM            255

#define LV_DRAW_BUF_BASIC           0
#define LV_DRAW_BUF_CUSTOM          255

/**********************
 *      TYPEDEFS
 **********************/

/**
 * LVGL error codes.
 */
enum _lv_res_t {
    LV_RES_INV = 0, /*Typically indicates that the object is deleted (become invalid) in the action
                      function or an operation was failed*/
    LV_RES_OK,      /*The object is valid (no deleted) after the action*/
};

#ifdef DOXYGEN
typedef _lv_res_t lv_res_t;
#else
typedef uint8_t lv_res_t;
#endif /*DOXYGEN*/


#if defined(__cplusplus) || __STDC_VERSION__ >= 199901L
// If c99 or newer,  use the definition of uintptr_t directly from <stdint.h>
typedef uintptr_t lv_uintptr_t;
typedef intptr_t lv_intptr_t;

#else

// Otherwise, use the arch size determination
#ifdef LV_ARCH_64
typedef uint64_t lv_uintptr_t;
typedef int64_t lv_intptr_t;
#else
typedef uint32_t lv_uintptr_t;
typedef int32_t lv_intptr_t;
#endif

#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#define LV_UNUSED(x) ((void)x)

#define _LV_CONCAT(x, y) x ## y
#define LV_CONCAT(x, y) _LV_CONCAT(x, y)

#define _LV_CONCAT3(x, y, z) x ## y ## z
#define LV_CONCAT3(x, y, z) _LV_CONCAT3(x, y, z)

#if defined(PYCPARSER) || defined(__CC_ARM)
#define LV_FORMAT_ATTRIBUTE(fmtstr, vararg)
#elif defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 4) || __GNUC__ > 4)
#define LV_FORMAT_ATTRIBUTE(fmtstr, vararg) __attribute__((format(gnu_printf, fmtstr, vararg)))
#elif (defined(__clang__) || defined(__GNUC__) || defined(__GNUG__))
#define LV_FORMAT_ATTRIBUTE(fmtstr, vararg) __attribute__((format(printf, fmtstr, vararg)))
#else
#define LV_FORMAT_ATTRIBUTE(fmtstr, vararg)
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TYPES_H*/
