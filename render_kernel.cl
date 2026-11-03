
#define FLAG_SCALE_TO_WINDOW  (1u << 0)
#define FLAG_SOLID_COLOUR     (1u << 1)
#define USE_GLOBAL_SCALE      (1u << 2)
#define USE_CAM_COORDS        (1u << 3)
#define ROUND_IMAGE           (1u << 4)

#define ROUND_RADIUS          8.0f // corner radius in logical pixels

#define THREADS_PER_TEX       (1u << 5) // 32

typedef struct
{
    int    x;
    int    y;
    float  scale;

    ushort textureId;

    uchar  depth;
    uchar  flags;

    uint   solidWidth;
    uint   solidHeight;

} Sprite;


__kernel void render_textures(

    // --------------------------------------------------------
    // Shared texture data
    // --------------------------------------------------------

    __global const uchar *textureData,
    __global const uint  *textureOffsets,
    __global const uint  *textureWidths,
    __global const uint  *textureHeights,

    // --------------------------------------------------------
    // Sprite data
    // --------------------------------------------------------

    __global const Sprite *sprites,

    // --------------------------------------------------------
    // Camera
    // --------------------------------------------------------

    const int camera_x,
    const int camera_y,

    // --------------------------------------------------------
    // Solid colours
    // --------------------------------------------------------

    __global const uchar *solidColours,

    // --------------------------------------------------------
    // Depth buffer
    // --------------------------------------------------------

    __global uint *depthBuffer,

    // --------------------------------------------------------
    // Output
    // --------------------------------------------------------

    write_only image2d_t backbuffer,

    const uint backbufferWidth,
    const uint backbufferHeight,

    // --------------------------------------------------------
    // Logical/reference resolution
    // --------------------------------------------------------

    const uint screenWidth,
    const uint screenHeight,

    const float global_scale
)
{
    // ========================================================
    // WORK ITEM
    // ========================================================

    const uint spriteIndex = get_group_id(0);
    const uint lane        = get_local_id(1);


    // ========================================================
    // LOAD SPRITE
    // ========================================================

    const Sprite sprite = sprites[spriteIndex];

    const ushort texID = sprite.textureId;
    const uchar  flags = sprite.flags;


    // ========================================================
    // FLAGS
    // ========================================================

    const bool scaleToWindow =
        (flags & FLAG_SCALE_TO_WINDOW) != 0;

    const bool solidColour =
        (flags & FLAG_SOLID_COLOUR) != 0;

    const bool useGlobalScale =
        (flags & USE_GLOBAL_SCALE) != 0;
    
    const bool useCamOffset =
        (flags & USE_CAM_COORDS) != 0;

    const bool roundImage =
        (flags & ROUND_IMAGE) != 0;


    // ========================================================
    // SOURCE DIMENSIONS
    // ========================================================

    uint sourceWidth;
    uint sourceHeight;

    if (solidColour)
    {
        sourceWidth  = sprite.solidWidth;
        sourceHeight = sprite.solidHeight;
    }
    else
    {
        sourceWidth  = textureWidths[texID];
        sourceHeight = textureHeights[texID];
    }

    if (sourceWidth == 0 ||
        sourceHeight == 0)
    {
        return;
    }


    // ========================================================
    // SPRITE POSITION / DEPTH
    // ========================================================

    const int spriteX = sprite.x;
    const int spriteY = sprite.y;

    const uchar spriteDepth = sprite.depth;


    // ========================================================
    // WINDOW SCALE
    // ========================================================

    float windowScaleX = 1.0f;
    float windowScaleY = 1.0f;

    if (scaleToWindow)
    {
        windowScaleX =
            (float)backbufferWidth /
            (float)screenWidth;

        windowScaleY =
            (float)backbufferHeight /
            (float)screenHeight;
    }


    // ========================================================
    // SPRITE SCALE
    // ========================================================

    float scaleX = sprite.scale;
    float scaleY = sprite.scale;

    if (useGlobalScale)
    {
        scaleX *= global_scale;
        scaleY *= global_scale;
    }

    if (scaleX <= 0.0f ||
        scaleY <= 0.0f)
    {
        return;
    }


    // ========================================================
    // TOTAL SCALE
    // ========================================================

    const float totalScaleX =
        scaleX * windowScaleX;

    const float totalScaleY =
        scaleY * windowScaleY;

    if (totalScaleX <= 0.0f ||
        totalScaleY <= 0.0f)
    {
        return;
    }


    // ========================================================
    // DESTINATION SIZE
    // ========================================================

    const uint dstWidth =
        (uint)ceil(
            (float)sourceWidth *
            totalScaleX
        );

    const uint dstHeight =
        (uint)ceil(
            (float)sourceHeight *
            totalScaleY
        );

    if (dstWidth == 0 ||
        dstHeight == 0)
    {
        return;
    }

    const uint pixelCount =
        dstWidth * dstHeight;


    // ========================================================
    // ROUND EDGE RADIUS
    // ========================================================

    float cornerRadius = 0.0f;

    if (roundImage)
    {
        cornerRadius =
            ROUND_RADIUS *
            fmin(totalScaleX, totalScaleY);

        cornerRadius = fmin(
            cornerRadius,
            0.5f * (float)min(dstWidth, dstHeight)
        );
    }


    // ========================================================
    // SPRITE SCREEN POSITION
    // ========================================================

    float screenOriginX =
        (int)(
            (float)(spriteX - ((useCamOffset) ? camera_x : 0)) *
            windowScaleX
        );

    float screenOriginY =
        (int)(
            (float)(spriteY - ((useCamOffset) ? camera_y : 0)) *
            windowScaleY
        );

    int screen_orx = 0;
    int screen_ory = 0;

    if (useGlobalScale){
        screen_orx = ceil(global_scale * screenOriginX);
        screen_ory = ceil(global_scale * screenOriginY);
    }
    else{
        screen_orx = ceil(screenOriginX);
        screen_ory = ceil(screenOriginY);
    }

    // ========================================================
    // SOLID COLOUR
    // ========================================================

    float4 colour = (float4)(0.0f);

    if (solidColour)
    {
        const uint colourOffset =
            (uint)texID * 3u;

        const uchar r =
            solidColours[colourOffset + 0];

        const uchar g =
            solidColours[colourOffset + 1];

        const uchar b =
            solidColours[colourOffset + 2];

        colour = (float4)(
            (float)r * (1.0f / 255.0f),
            (float)g * (1.0f / 255.0f),
            (float)b * (1.0f / 255.0f),
            1.0f
        );
    }


    // ========================================================
    // RENDER PIXELS
    // ========================================================

    for (uint dstIndex = lane;
         dstIndex < pixelCount;
         dstIndex += THREADS_PER_TEX)
    {
        // ----------------------------------------------------
        // Linear index -> destination X/Y
        // ----------------------------------------------------

        const uint dstY =
            dstIndex / dstWidth;

        const uint dstX =
            dstIndex -
            dstY * dstWidth;


        // ----------------------------------------------------
        // Destination screen position
        // ----------------------------------------------------

        const int screenX =
            screen_orx +
            (int)dstX;

        const int screenY =
            screen_ory +
            (int)dstY;


        // ----------------------------------------------------
        // Screen clipping
        // ----------------------------------------------------

        if (screenX < 0 ||
            screenY < 0 ||
            screenX >= (int)backbufferWidth ||
            screenY >= (int)backbufferHeight)
        {
            continue;
        }


        // ----------------------------------------------------
        // Round edges
        // ----------------------------------------------------
        //
        // Skip pixels whose centre is further than the radius
        // from the inner rectangle (the sprite shrunk by the
        // radius on every side).
        //

        if (roundImage)
        {
            const float px = (float)dstX + 0.5f;
            const float py = (float)dstY + 0.5f;

            const float cx = clamp(
                px,
                cornerRadius,
                (float)dstWidth - cornerRadius
            );

            const float cy = clamp(
                py,
                cornerRadius,
                (float)dstHeight - cornerRadius
            );

            const float dx = px - cx;
            const float dy = py - cy;

            if (dx * dx + dy * dy > cornerRadius * cornerRadius)
                continue;
        }


        // ====================================================
        // NORMAL TEXTURE
        // ====================================================

        if (!solidColour)
        {
            // ------------------------------------------------
            // Bilinear sampling
            // ------------------------------------------------
            //
            // Map the destination pixel centre back into source
            // space, then blend the 4 nearest texels. Edges are
            // clamped so sprites don't bleed past their bounds.
            //

            float u =
                ((float)dstX + 0.5f) / totalScaleX - 0.5f;

            float v =
                ((float)dstY + 0.5f) / totalScaleY - 0.5f;

            u = clamp(u, 0.0f, (float)(sourceWidth  - 1));
            v = clamp(v, 0.0f, (float)(sourceHeight - 1));

            const uint x0 = (uint)u;
            const uint y0 = (uint)v;

            const uint x1 = min(x0 + 1u, sourceWidth  - 1u);
            const uint y1 = min(y0 + 1u, sourceHeight - 1u);

            const float fx = u - (float)x0;
            const float fy = v - (float)y0;


            // ------------------------------------------------
            // Fetch the 4 RGBA8 texels
            // ------------------------------------------------

            __global const uchar *tex =
                textureData + textureOffsets[texID];

            const float4 t00 = convert_float4(
                vload4(0, tex + (y0 * sourceWidth + x0) * 4));

            const float4 t10 = convert_float4(
                vload4(0, tex + (y0 * sourceWidth + x1) * 4));

            const float4 t01 = convert_float4(
                vload4(0, tex + (y1 * sourceWidth + x0) * 4));

            const float4 t11 = convert_float4(
                vload4(0, tex + (y1 * sourceWidth + x1) * 4));


            // ------------------------------------------------
            // Blend with premultiplied alpha
            // ------------------------------------------------
            //
            // Premultiplying stops fully transparent texels
            // (often black) darkening the edges of a sprite.
            //

            const float4 p00 = (float4)(t00.xyz * t00.w, t00.w);
            const float4 p10 = (float4)(t10.xyz * t10.w, t10.w);
            const float4 p01 = (float4)(t01.xyz * t01.w, t01.w);
            const float4 p11 = (float4)(t11.xyz * t11.w, t11.w);

            const float4 top    = mix(p00, p10, fx);
            const float4 bottom = mix(p01, p11, fx);
            const float4 texel  = mix(top, bottom, fy);

            const float a = texel.w;

            if (a < 5.0f)
                continue;

            colour = (float4)(
                texel.xyz / a,
                a
            ) * (1.0f / 255.0f);
        }


        // ====================================================
        // ATOMIC DEPTH TEST + WRITE
        // ====================================================
        //
        // Depth (top 8 bits) and RGB (low 24 bits) are packed
        // into one uint and written with a single atomic_max,
        // so the depth test and colour write can't be split
        // by another thread. resolve_depth copies the result
        // into the backbuffer afterwards.
        //

        const uint depthIndex =
            (uint)screenY * backbufferWidth +
            (uint)screenX;

        const uint4 c = convert_uint4_sat_rte(colour * 255.0f);

        const uint packed =
            ((uint)spriteDepth << 24) |
            (c.x << 16) | (c.y << 8) | c.z;

        if (spriteDepth > 0)
            atomic_max(&depthBuffer[depthIndex], packed);
    }
}

// Copies packed depth/colour values into the backbuffer.
// Pixels with value 0 were never drawn and keep the clear colour.
__kernel void resolve_depth(
    __global const uint *depthBuffer,
    write_only image2d_t backbuffer,
    const uint backbufferWidth,
    const uint backbufferHeight
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);

    if (x >= backbufferWidth || y >= backbufferHeight)
        return;

    const uint v = depthBuffer[y * backbufferWidth + x];

    if (v == 0)
        return;

    const float4 colour = (float4)(
        (float)((v >> 16) & 0xFF),
        (float)((v >> 8)  & 0xFF),
        (float)( v        & 0xFF),
        255.0f
    ) * (1.0f / 255.0f);

    write_imagef(backbuffer, (int2)(x, y), colour);
}
