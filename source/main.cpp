
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../contrib/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../contrib/stb_image.h"

#include <stdio.h>
#include <stdint.h>

typedef uint32_t uint;

#define arrsize(_arr) ((sizeof(_arr) / sizeof(_arr[0])))

union Color
{
    uint32_t value;

    struct
    {
        uint8_t r, g, b, a;
    };
};

struct Bitmap
{
    Color* image;
    uint w;
    uint h;
};

void Clear(Bitmap bitmap)
{
    memset(bitmap.image, 0, bitmap.w * bitmap.h * sizeof(Color));
}

struct Grid
{
    uint cell_width;
    uint cell_height;

    uint grid_width;
    uint grid_height;

    uint PixelWidth() { return cell_width * grid_width; }
    uint PixelHeight() { return cell_height * grid_height; }
};


void DrawBox(Bitmap bitmap, Color color, uint x, uint y, uint w, uint h)
{
    for (uint dest_y = y; dest_y < y + h; ++dest_y)
    {
        for (uint dest_x = x; dest_x < x + w; ++dest_x)
        {
            Color* dest = bitmap.image + dest_y * bitmap.w + dest_x;
            *dest = color;
        }
    }
}

void DrawGrid(Grid& grid, Bitmap bitmap, Color color)
{
    for (uint x = grid.cell_width - 1; x < grid.PixelWidth(); x += grid.cell_width)
    {
        DrawBox(bitmap, color, x, 0, 1, grid.PixelHeight());
    }

    for (uint y = grid.cell_height - 1; y < grid.PixelHeight(); y += grid.cell_height)
    {
        DrawBox(bitmap, color, 0, y, grid.PixelWidth(), 1);
    }
}

void DrawHelperOverlay(Grid grid, Bitmap bitmap)
{
    enum Loc
    {
        Loc_Left,
        Loc_BotLeft,
        Loc_Bot,
        Loc_BotRight,
        Loc_Right,
        Loc_Top,

        Loc_Count
    };

    struct UV
    {
        float x;
        float y;
    };

    float min = 0.1f;
    float max = 0.9f;
    float mid = 0.5f;

    UV helper_locations[] = {
        { min, mid }, // Loc_Left,
        { min, min }, // Loc_BotLeft,
        { mid, min }, // Loc_Bot,
        { max, min }, // Loc_BotRight,
        { max, mid }, // Loc_Right,
        { mid, max }, // Loc_Top,
    };
    static_assert(arrsize(helper_locations) == Loc_Count, "");
    Color colors[] = {
        { 0xFF0000FF },
        { 0xFF00FF00 },
        { 0xFFFF0000 },
        { 0xFF00FFFF },
        { 0xFFFF00FF },
        { 0xFFFFFF00 },
    };
    static_assert(arrsize(colors) == Loc_Count, "");

    uint block_features = 0;
    for (uint y = 0; y < grid.grid_height; ++y)
    {
        for (uint x = 0; x < grid.grid_width; ++x)
        {
            for (uint feature = 0; feature < Loc_Count; ++feature)
            {
                if ((block_features & (1 << feature)) != 0)
                {
                    UV location = helper_locations[feature];
                    uint hx = x * grid.cell_width + uint(location.x * grid.cell_width);
                    uint hy = y * grid.cell_height + uint(location.y * grid.cell_height);
                    DrawBox(bitmap, colors[feature], hx, hy, 2, 2);
                }
            }
            block_features++;
        }
    }

}

void FillWithDefault(Grid grid, Bitmap bitmap, const char* filename)
{
    int x, y, n;
    Color* image = (Color*)stbi_load(filename, &x, &y, &n, 4);
    if (image == nullptr)
        return;
    if (x != grid.cell_width || y != grid.cell_height)
        return;

    for (uint y = 0; y < grid.grid_height; ++y)
    {
        for (uint x = 0; x < grid.grid_width; ++x)
        {
            for (uint cy = 0; cy < grid.cell_height; ++cy)
            {
                Color* dest = bitmap.image + (y * grid.cell_height + cy) * grid.PixelWidth() + x * grid.cell_width;
                Color* source = image + cy * grid.cell_width;
                memcpy(dest, source, grid.cell_width * sizeof(Color));
            }
        }
    }

    stbi_image_free(image);
}

void Usage()
{
    printf("Gridsoft.exe!!\n");
    printf("\t-o \"output_filename.png\"\n");
    printf("\t-g Draw a grid\n");
    printf("\t-h Draw helper overlay\n");
    printf("\t-f \"filename.png\" Fill with this image\n");
}

int main(int argc, char** argv)
{
    const char* output_filename = nullptr;
    const char* fill_bitmap = nullptr;
    bool draw_grid = false;
    bool draw_helpers = false;

    for (int i = 1; i < argc; ++i)
    {
        char* arg = argv[i];
        bool has_next = i + 1 < argc;
        char* next = has_next ? argv[i + 1] : nullptr;

        if (strcmpi(arg, "-o") == 0)
        {
            output_filename = next;
            ++i;
        }
        else if (strcmpi(arg, "-g") == 0)
        {
            draw_grid = true;
        }
        else if (strcmpi(arg, "-f") == 0)
        {
            fill_bitmap = next;
            ++i;
        }
        else if (strcmpi(arg, "-h") == 0)
        {
            draw_helpers = true;
        }
    }

    if (output_filename == nullptr)
    {
        Usage();
        return 1;
    }

    Grid grid;
    grid.grid_width  = 8;
    grid.grid_height = 8;
    grid.cell_width  = 32;
    grid.cell_height = 32;

    Bitmap bitmap = {};
    bitmap.image = new Color[grid.PixelWidth() * grid.PixelHeight()];
    bitmap.w = grid.PixelWidth();
    bitmap.h = grid.PixelHeight();
    Color line_color;
    line_color.value = 0xFF00FF00;

    Clear(bitmap);
    if (fill_bitmap)
        FillWithDefault(grid, bitmap, fill_bitmap);
    if (draw_grid)
        DrawGrid(grid, bitmap, line_color);
    if (draw_helpers)
        DrawHelperOverlay(grid, bitmap);

    stbi_write_png(output_filename, bitmap.w, bitmap.h, 4, bitmap.image, bitmap.w * sizeof(Color));
    return 0;
}

