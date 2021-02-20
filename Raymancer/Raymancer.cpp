// Raymancer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>

#define M_PI 3.14159265358979323846264338327950288

/*
    Draws a rectangle on the passed vector representing an image
*/
void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, const size_t img_h, const size_t x, const size_t y, const size_t w, const size_t h, const uint32_t color){
    assert(img.size() == img_w * img_h);
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            size_t cx = x + i;
            size_t cy = y + j;
            if (cx >= img_w || cy >= img_h)continue;
            img[cx + cy * img_w] = color;
        }
    }
}

/*
    packs four 8-bit color traits into a single 32-bit color
*/
uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255){
    return ((a<<24)+(b<<16)+(g<<8)+(r));
}

/*
    unpacks 32-bit color representation into 4 8-bit color traits
*/
void unpack_color(const uint32_t& color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
    a = (color >> 24) & 255;
    b = (color>>16) & 255;
    g = (color>>8) & 255;
    r = (color) & 255;
}

/*
    saves .ppm file which is a graphic representing a passed vector of colors
*/
void drop_ppm_image(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h){
    assert(image.size() == w*h);

    std::ofstream ofs;
    ofs.open(filename, std::ofstream::out | std::ofstream::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for (int i = 0; i < w * h; i++) {
        uint8_t r, g, b, a;
        unpack_color(image[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
    }
    ofs.close();
}

int main()
{
    const size_t win_w = 1024;//image width
    const size_t win_h = 512;//image height
    std::vector<uint32_t> framebuffer(win_w*win_h, 255);
    std::vector<uint32_t> screenBuffer(win_w * win_h, 255);


    const size_t map_w = 16;
    const size_t map_h = 16;
    const char map[] =  "0000222222220000"\
                        "1              0"\
                        "1      11111   0"\
                        "1     0        0"\
                        "0     0  1110000"\
                        "0     3        0"\
                        "0   10000      0"\
                        "0   0   11100  0"\
                        "0   0   0      0"\
                        "0   0   1  00000"\
                        "0       1      0"\
                        "2       1      0"\
                        "0       0      0"\
                        "0 0000000      0"\
                        "0              0"\
                        "0002222222200000"; // our game map
    assert(sizeof(map) == map_w * map_h + 1);//+1 for null terminated string

    float player_x = 3.456f;
    float player_y = 2.345f;
    float player_a = 1.523f;
    float fov = M_PI/3;

    //---------------------SETUP COLORS---------------------
    size_t nColors = 10;
    std::vector<uint32_t> colors(nColors);
    for (int i = 0; i < nColors; i++) {
        colors[i] = pack_color(rand() % 255, rand() % 255, rand() % 255);
    }

    //--------------------initialize map and player view arrays--------------------
    for (size_t j = 0; j < win_h; j++) {
        for (size_t i = 0; i < win_w; i++) {
            uint8_t r = 255 * j / float(win_h);//j/win_h gives percent across, multiplied by full brightness. increasing percentage of brightness across.
            uint8_t g = 255 * i / float(win_w);
            uint8_t b = 0;
            framebuffer[i + j * win_w] = pack_color(r, g, b);
            screenBuffer[i + j * win_w] = pack_color(255,255,255);
        }
    }

    //-------------------------DRAW EACH MAP SPACE TO MAP GRAPHIC-------------------
    size_t rect_w = win_w / map_w;//pixel width of each square of the map applied to the image
    size_t rect_h = win_h / map_h;
    for (int j = 0; j < map_h; j++) {//for each map position
        for (int i = 0; i < map_w; i++) {
            if (map[i + j * map_w] == ' ')continue; //skip every empty space on the map

            size_t rect_x = i * rect_w;//iteration multiplied by the pixel width of a map tile
            size_t rect_y = j * rect_h;

            draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, pack_color(0, 255, 255));
        }
    }

    //Draw player on map
    draw_rectangle(framebuffer, win_w, win_h, player_x*rect_w, player_y*rect_h, 5,5, pack_color(255,255,255));

    //--------------------------RAYCAST FROM PLAYER VIEW-----------------------
    for (float i = 0; i < win_w; i++) {//do for window width so that you have a ray for every horizontal pixel after
        float t = 0.0f;
        for (; t < 20.0f; t += 0.05f) {
            float angle = player_a - (fov / 2) + fov * (i / win_w);//start at player angle - half fov, then add 
            float cx = player_x + t * cos(angle);
            float cy = player_y + t * sin(angle);
            if (map[(int)cx + (int)cy * map_w] != ' ')break;

            //printf("%f\n", t);
            size_t pix_x = cx * rect_w;
            size_t pix_y = cy * rect_h;
            framebuffer[pix_x + pix_y * win_w] = pack_color(255, 255, 255);            
        }

        float vertLength = win_h / (t + 0.01f);
        printf("%f - %f C:%f\n", vertLength, i, (t * 10) / 255);
        //if (vertLength > win_h)vertLength = win_h;
        float startPos = win_h / 2 - vertLength / 2;
        draw_rectangle(screenBuffer, win_w, win_h, i, startPos, 1, vertLength, pack_color(0, (t*10), 255));
    }

    //create images
    drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);
    drop_ppm_image("./firstPerson.ppm", screenBuffer, win_w, win_h);
    
    return 0;
}
