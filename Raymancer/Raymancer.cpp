// Raymancer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

/*
* Extracts a column from a horizontal position of the desired texture in the passed image and stretches it to column_height 
    img: texture image
	texsize: image width or height (square, so same)
	ntextures: number of textures in image
	texid: which texture to be used
	texcoord: which column of texture to be used
	column_height: height of wall pixels to be drawn
*/
std::vector<uint32_t> texture_column(const std::vector<uint32_t>& img, const size_t texsize, const size_t ntextures, const size_t texid, const size_t texcoord, const size_t column_height) {
    //Full image width should be texsize multiplied by amount of textures
    const size_t img_w = texsize * ntextures;
    const size_t img_h = texsize;
    assert(img.size() == img_w * img_h && texcoord < texsize&& texid < ntextures);

    std::vector<uint32_t> column(column_height);
    
    for (int y = 0; y < column_height; y++) {
        size_t pix_x = texid * texsize + texcoord;
        size_t pix_y = y * texsize / column_height;

        column[y] = img[pix_x + pix_y * img_w];
    }
    return column;
}

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
    Load texture from image file using the public stbi library
*/
bool load_texture(const std::string filename, std::vector<uint32_t>& texture, size_t& text_size, size_t& text_cnt) {
    int nchannels = -1, w, h;

    unsigned char* pixmap = stbi_load(filename.c_str(), &w, &h, &nchannels, 0);
    if (!pixmap) {
        std::cerr << "Unable to load texture: " << filename << std::endl;
        return false;
    }

    if (nchannels != 4) {
        std::cerr << "Error: Texture file " << filename << " must be a 32-bit image." << std::endl;
        stbi_image_free(pixmap);
        return false;
    }

    text_cnt = w / h;
    text_size = w / text_cnt;
    if (w != h * (int)text_cnt) {
        std::cerr << "Error: The texture file must be N square textures packed horizontally." << std::endl;
        stbi_image_free(pixmap);
        return false;
    }

    texture = std::vector<uint32_t>(w * h);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint8_t r = pixmap[(i + j * w) * 4 + 0];
            uint8_t g = pixmap[(i + j * w) * 4 + 1];
            uint8_t b = pixmap[(i + j * w) * 4 + 2];
            uint8_t a = pixmap[(i + j * w) * 4 + 3];
            texture[i + j * w] = pack_color(r, g, b, a);
        }
    }

    stbi_image_free(pixmap);
    return true;
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
                        "0   3   11100  0"\
                        "5   4   0      0"\
                        "5   4   1  00000"\
                        "0       1      0"\
                        "2       1      0"\
                        "0       0      0"\
                        "0 0000000      0"\
                        "0              0"\
                        "0002222222200000"; // game map
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

    //--------------------LOAD TEXTURES---------------------
    std::vector<uint32_t> wallText;
    size_t wallText_size;//texture dimensions (square)
    size_t wallText_cnt;//number of different textures
    if (!load_texture("walltext.png", wallText, wallText_size, wallText_cnt)) {
        std::cerr << "Failed to load texture." << std::endl;
        return -1;
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

            size_t texid = map[i+j*map_w]-'0';
            assert(texid < wallText_cnt);

            draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, wallText[texid*wallText_size]);//texid*wallText_size = id*width to get to the first pixel of the one you want
        }
    }

    //Draw player on map
    draw_rectangle(framebuffer, win_w, win_h, player_x*rect_w, player_y*rect_h, 5,5, pack_color(255,255,255));

    //--------------------------RAYCAST FROM PLAYER VIEW-----------------------
    for (int frame = 1; frame < 360; frame++) {
        player_a += 2*M_PI/360;

        screenBuffer = std::vector<uint32_t>(win_w * win_h, pack_color(255, 255, 255));        

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << frame << ".ppm";
        //printing current output
        std::cout << ss.str() << std::endl;

        for (float i = 0; i < win_w; i++) {//do for window width so that you have a ray for every horizontal pixel after
            float t = 0.0f;
            float angle;
            float cx, cy;
            size_t pix_x, pix_y;

            for (; t < 20.0f; t += 0.01f) {
                angle = player_a - (fov / 2) + fov * (i / win_w);//start at player angle - half fov, then add 
                cx = player_x + t * cos(angle);
                cy = player_y + t * sin(angle);
                if (map[(int)cx + (int)cy * map_w] != ' ')break;                

                pix_x = cx * rect_w;
                pix_y = cy * rect_h;
                framebuffer[pix_x + pix_y * win_w] = pack_color(255, 255, 255);//Drawing visual rays
            }

            //-----------------FIND TEXTURE TEXTURE COORDINATE POSITION-----------------------
            float hitx = cx - floor(cx + 0.5);//subtract the nearest whole to get the distance to the nearest whole
            float hity = cy - floor(cy + 0.5);
            int x_texcoord;
            if(std::abs(hitx)>std::abs(hity)){//compare absolute values to see which one is further from a whole. this is the axis which the face is on.
                x_texcoord = hitx * wallText_size;
            }else{
                x_texcoord = hity * wallText_size;
            }
            //position may be negative, and must be made positive to reference texture position
            if (x_texcoord < 0) x_texcoord += wallText_size;
            assert(x_texcoord>=0 && x_texcoord<(int)wallText_size);

            //get texture id from current wall collision
            size_t texid = map[(int)cx + (int)cy * map_w] - '0';
            assert(texid < wallText_cnt);

            size_t column_height = win_h / (t * cos(angle - player_a));

            std::vector<uint32_t> column = texture_column(wallText, wallText_size, wallText_cnt, texid, x_texcoord, column_height);

            pix_x = i;
            for (size_t j = 0; j < column_height; j++) {
                pix_y = j + win_h / 2 - column_height / 2;
                if (pix_y < 0 || pix_y >= (int)win_w)continue;
                screenBuffer[pix_x + pix_y * win_w] = column[j];
            }
        }

        //create player view file
        drop_ppm_image(ss.str(), screenBuffer, win_w, win_h);
    }

    const size_t texid = 4;
    for (size_t i = 0; i < wallText_size; i++) {
        for (size_t j = 0; j < wallText_size; j++) {
            framebuffer[i + j * win_w] = wallText[i + texid * wallText_size + j * wallText_size * wallText_cnt];
        }
    }

    //create map image file
    drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);
    
    return 0;
}
