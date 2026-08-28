#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 16 (RGB565) */
#define LV_COLOR_DEPTH 16
/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (SPI, etc.) or M5GFX */
#define LV_COLOR_16_SWAP 1

/* Memory manager */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128U * 1024U)

/* Display refresh settings */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Feature configuration */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Fonts */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_18

/* Enable Tiny_TTF for dynamic TrueType Font (.ttf) loading */
#define LV_USE_TINY_TTF 1

/* Enable Japanese / UTF-8 font support */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Widgets */
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_TABVIEW 1

/* Extra layouts & animations */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_USE_ANIMATION 1

#endif /* LV_CONF_H */
