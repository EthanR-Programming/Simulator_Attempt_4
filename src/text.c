#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "text.h"

struct font_s {
    stbtt_fontinfo info;
    float scale;
};
struct font_s font;

void font_init(unsigned char *ttf_buffer, int height){
    int offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);

    if (offset < 0)
        return;

    if (!stbtt_InitFont(&font.info, ttf_buffer, offset))
        return;

    font.scale =
        stbtt_ScaleForPixelHeight(&font.info, (float)height);

    return;
}

unsigned char *text_load_char(
    unsigned char *ttf_buffer,
    int *width,
    int *height,
    unsigned char character,
    int pixel_height,
    int *advance,
    int *yoffset,
    int *xoffset,
    unsigned char colour[3]
)
{

    // Scale
    float stb_scale = font.scale;

    // Get bitmap
    int w;
    int h;
    int xoff;
    int yoff;

    unsigned char *bitmap =
        stbtt_GetCodepointBitmap(
            &font.info,
            stb_scale,
            stb_scale,
            character,
            &w,
            &h,
            &xoff,
            &yoff
        );

    if (bitmap == NULL)
    {

        int local_advance;
        int local_lsb;

        stbtt_GetCodepointHMetrics(
            &font.info,
            character,
            &local_advance,
            &local_lsb
        );

        *width = w;
        *height = h;   
        *advance = (int)(local_advance * stb_scale + 0.5f);
        *yoffset = yoff;
        *xoffset = xoff;

        //printf("Failed to generate glyph bitmap %c / %d\n" , character , character);
        return NULL;
    }

    // Get advances
    int advance_local;
    int lsb_local;

    stbtt_GetCodepointHMetrics(
        &font.info,
        character,
        &advance_local,
        &lsb_local
    );

    // Get pixel advance
    int pixel_advance =
        (int)(advance_local * stb_scale + 0.5f);


    // Allocate tex
    size_t rgba_size =
        (size_t)w * (size_t)h * 4;

    unsigned char *rgba =
        mem_malloc(rgba_size);

    if (rgba == NULL)
    {
        stbtt_FreeBitmap(bitmap, NULL);
        return NULL;
    }

    // Make BITMAP
    for (int y = 0; y < h; y++)
    {
        const int src_y =
            h - 1 - y;

        for (int x = 0; x < w; x++)
        {
            const unsigned char value =
                bitmap[src_y * w + x];

            const size_t dst =
                ((size_t)y * w + x) * 4;

            rgba[dst + 0] = colour[0];
            rgba[dst + 1] = colour[1];
            rgba[dst + 2] = colour[2];
            rgba[dst + 3] = value;
        }
    }

    //Set width height advance yoffset xoffset
    *width = w;
    *height = h;   
    *advance = pixel_advance;
    *yoffset = yoff;
    *xoffset = xoff;


    // free bitmap
    stbtt_FreeBitmap(bitmap, NULL);

    return rgba;
}