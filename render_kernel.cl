
#define FLAG_SCALE_TO_WINDOW  (1u << 0)
#define FLAG_SOLID_COLOUR     (1u << 1)
#define USE_GLOBAL_SCALE      (1u << 2)
#define USE_CAM_COORDS        (1u << 3)

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


        // ====================================================
        // NORMAL TEXTURE
        // ====================================================

        if (!solidColour)
        {
            // ------------------------------------------------
            // Nearest-neighbour sampling
            // ------------------------------------------------

            uint srcX =
                (uint)(
                    (float)dstX /
                    totalScaleX
                );

            uint srcY =
                (uint)(
                    (float)dstY /
                    totalScaleY
                );

            if (srcX >= sourceWidth)
                srcX = sourceWidth - 1;

            if (srcY >= sourceHeight)
                srcY = sourceHeight - 1;


            // ------------------------------------------------
            // Texture offset
            // ------------------------------------------------

            const uint textureOffset =
                textureOffsets[texID];


            // ------------------------------------------------
            // RGBA8 pixel
            // ------------------------------------------------

            const uint srcIndex =
                textureOffset +
                ((srcY * sourceWidth + srcX) * 4);


            const uchar r =
                textureData[srcIndex + 0];

            const uchar g =
                textureData[srcIndex + 1];

            const uchar b =
                textureData[srcIndex + 2];

            const uchar a =
                textureData[srcIndex + 3];

            if (a < 5)
                continue;


            colour = convert_float4(
                (uchar4)(r, g, b, a)
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
