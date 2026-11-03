
#define FLAG_SCALE_TO_WINDOW  (1u << 0)
#define FLAG_SOLID_COLOUR     (1u << 1)
#define USE_GLOBAL_SCALE      (1u << 2)
#define USE_CAM_COORDS        (1u << 3)
#define ROUND_IMAGE           (1u << 4)

#define ROUND_RADIUS          8.0f // corner radius in logical pixels

#define THREADS_PER_TEX       (1u << 5) // 32

#define MAX_SAMPLES           8u    // max samples per axis when shrinking a texture
#define OPAQUE_ALPHA          0.99f // pixels below this alpha get blended (edge buffer)

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


// Bilinear sample at source position (u, v), returns premultiplied RGBA (0-255).
// Premultiplying stops fully transparent texels (often black) darkening the
// edges of a sprite.
float4 sample_bilinear(
    __global const uchar *tex,
    const uint width,
    const uint height,
    float u,
    float v
)
{
    u = clamp(u, 0.0f, (float)(width  - 1));
    v = clamp(v, 0.0f, (float)(height - 1));

    const uint x0 = (uint)u;
    const uint y0 = (uint)v;

    const uint x1 = min(x0 + 1u, width  - 1u);
    const uint y1 = min(y0 + 1u, height - 1u);

    const float fx = u - (float)x0;
    const float fy = v - (float)y0;

    const float4 t00 = convert_float4(vload4(0, tex + (y0 * width + x0) * 4));
    const float4 t10 = convert_float4(vload4(0, tex + (y0 * width + x1) * 4));
    const float4 t01 = convert_float4(vload4(0, tex + (y1 * width + x0) * 4));
    const float4 t11 = convert_float4(vload4(0, tex + (y1 * width + x1) * 4));

    const float4 p00 = (float4)(t00.xyz * t00.w, t00.w);
    const float4 p10 = (float4)(t10.xyz * t10.w, t10.w);
    const float4 p01 = (float4)(t01.xyz * t01.w, t01.w);
    const float4 p11 = (float4)(t11.xyz * t11.w, t11.w);

    const float4 top    = mix(p00, p10, fx);
    const float4 bottom = mix(p01, p11, fx);

    return mix(top, bottom, fy);
}

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

    const float global_scale,

    // --------------------------------------------------------
    // Edge buffer (partly transparent pixels, blended in resolve)
    // --------------------------------------------------------

    __global uint *edgeBuffer
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
    // SAMPLES PER PIXEL
    // ========================================================
    //
    // When a texture is shrunk (e.g. 256px glyphs drawn at 25px)
    // one destination pixel covers many source texels. Averaging
    // a grid of samples across that area stops thin strokes
    // vanishing and edges going jagged.
    //

    const uint samplesX =
        clamp((uint)ceil(1.0f / totalScaleX), 1u, MAX_SAMPLES);

    const uint samplesY =
        clamp((uint)ceil(1.0f / totalScaleY), 1u, MAX_SAMPLES);


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
        // Fade out pixels by how far their centre is past the
        // radius from the inner rectangle (the sprite shrunk by
        // the radius on every side), giving a smooth curve.
        //

        float coverage = 1.0f;

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

            const float dist = sqrt(dx * dx + dy * dy);

            coverage = clamp(cornerRadius - dist + 0.5f, 0.0f, 1.0f);

            if (coverage <= 0.0f)
                continue;
        }


        // ====================================================
        // NORMAL TEXTURE
        // ====================================================

        if (!solidColour)
        {
            // ------------------------------------------------
            // Filtered sampling
            // ------------------------------------------------
            //
            // Map a grid of points inside the destination pixel
            // back into source space and average their bilinear
            // samples (one sample when not shrinking).
            //

            __global const uchar *tex =
                textureData + textureOffsets[texID];

            float4 texel = (float4)(0.0f);

            for (uint sy = 0; sy < samplesY; sy++)
            {
                const float v =
                    ((float)dstY + ((float)sy + 0.5f) / (float)samplesY) /
                    totalScaleY - 0.5f;

                for (uint sx = 0; sx < samplesX; sx++)
                {
                    const float u =
                        ((float)dstX + ((float)sx + 0.5f) / (float)samplesX) /
                        totalScaleX - 0.5f;

                    texel += sample_bilinear(
                        tex, sourceWidth, sourceHeight, u, v);
                }
            }

            texel /= (float)(samplesX * samplesY);

            const float a = texel.w;

            if (a < 1.0f)
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

        // Partly transparent pixels (anti-aliased edges) go to
        // the edge buffer instead, packed as depth | alpha |
        // RGB565, and get blended over the opaque result in
        // resolve_depth.
        //

        if (spriteDepth == 0)
            continue;

        const uint depthIndex =
            (uint)screenY * backbufferWidth +
            (uint)screenX;

        const float4 pixel = (float4)(colour.xyz, colour.w * coverage);

        const uint4 c = convert_uint4_sat_rte(pixel * 255.0f);

        if (pixel.w >= OPAQUE_ALPHA)
        {
            const uint packed =
                ((uint)spriteDepth << 24) |
                (c.x << 16) | (c.y << 8) | c.z;

            atomic_max(&depthBuffer[depthIndex], packed);
        }
        else
        {
            const uint rgb565 =
                ((c.x >> 3) << 11) | ((c.y >> 2) << 5) | (c.z >> 3);

            const uint packed =
                ((uint)spriteDepth << 24) |
                (c.w << 16) | rgb565;

            atomic_max(&edgeBuffer[depthIndex], packed);
        }
    }
}

// Copies packed depth/colour values into the backbuffer, then blends
// the edge buffer pixel on top if it isn't hidden by an opaque pixel.
// Pixels never drawn keep the clear colour.
__kernel void resolve_depth(
    __global const uint *depthBuffer,
    write_only image2d_t backbuffer,
    const uint backbufferWidth,
    const uint backbufferHeight,
    __global const uint *edgeBuffer,
    const float4 clearColour
)
{
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);

    if (x >= backbufferWidth || y >= backbufferHeight)
        return;

    const uint index = y * backbufferWidth + x;

    const uint v = depthBuffer[index];
    const uint e = edgeBuffer[index];

    if (v == 0 && e == 0)
        return;

    float3 colour = clearColour.xyz;

    if (v != 0)
    {
        colour = (float3)(
            (float)((v >> 16) & 0xFF),
            (float)((v >> 8)  & 0xFF),
            (float)( v        & 0xFF)
        ) * (1.0f / 255.0f);
    }

    if (e != 0 && (e >> 24) >= (v >> 24))
    {
        const float alpha = (float)((e >> 16) & 0xFF) * (1.0f / 255.0f);

        const float3 edgeColour = (float3)(
            (float)((e >> 11) & 0x1F) * (1.0f / 31.0f),
            (float)((e >> 5)  & 0x3F) * (1.0f / 63.0f),
            (float)( e        & 0x1F) * (1.0f / 31.0f)
        );

        colour = mix(colour, edgeColour, alpha);
    }

    write_imagef(backbuffer, (int2)(x, y), (float4)(colour, 1.0f));
}
