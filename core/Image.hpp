#pragma once
#include "Vector.hpp"
#include "Utils.hpp"
#include <math.h>

class Image
{
public:
    int width, height, channels;
    unsigned char* ldr_buffer;

	Image(const string& fileName) 
    {
        load_tga_image(fileName.c_str());
    }
	~Image() 
    {
        free(this->ldr_buffer);
    }


private:
    void image_init() {
        int num_elems = width * height * channels;

        assert(width > 0 && height > 0 && channels >= 1 && channels <= 4);

        this->ldr_buffer = NULL;

        int size = sizeof(unsigned char) * num_elems;
        this->ldr_buffer = (unsigned char*)malloc(size);
        memset(this->ldr_buffer, 0, size);
    }

    int get_num_elems() {
        return this->width * this->height * this->channels;
    }

    void load_tga_rle_payload(FILE* file) {
        int num_elems = get_num_elems();
        int curr_size = 0;
        while (curr_size < num_elems) {
            unsigned char header = read_byte(file);
            int rle_packet = header & 0x80;
            int num_pixels = (header & 0x7F) + 1;
            unsigned char pixel[4];
            assert(curr_size + num_pixels * this->channels <= num_elems);
            if (rle_packet) {                                   /* rle packet */
                for (int j = 0; j < this->channels; j++) {
                    pixel[j] = read_byte(file);
                }
                for (int i = 0; i < num_pixels; i++) {
                    for (int j = 0; j < this->channels; j++) {
                        this->ldr_buffer[curr_size++] = pixel[j];
                    }
                }
            }
            else {                                            /* raw packet */
                for (int i = 0; i < num_pixels; i++) {
                    for (int j = 0; j < this->channels; j++) {
                        this->ldr_buffer[curr_size++] = read_byte(file);
                    }
                }
            }
        }
        assert(curr_size == num_elems);
    }

    void swap_bytes(unsigned char* a, unsigned char* b) {
        unsigned char t = *a;
        *a = *b;
        *b = t;
    }

    void swap_floats(float* a, float* b) {
        float t = *a;
        *a = *b;
        *b = t;
    }

    unsigned char* get_ldr_pixel( int row, int col) {
        int index = (row * this->width + col) * this->channels;
        return &this->ldr_buffer[index];
    }


    bool load_tga_image(const char* filename) {
        int is_rle, flip_h, flip_v;
        FILE* file;

        file = fopen(filename, "rb");
        assert(file != NULL);
        read_tga_header(file, &width, &height, &channels,
            &is_rle, &flip_h, &flip_v);
        image_init();
        if (is_rle) {
            load_tga_rle_payload(file);
        }
        else {
            read_bytes(file, this->ldr_buffer, get_num_elems());
        }
        fclose(file);

        if (channels >= 3) {
            int r, c;
            for (r = 0; r < this->height; r++) {
                for (c = 0; c < this->width; c++) {
                    unsigned char* pixel = get_ldr_pixel(r, c);
                    swap_bytes(&pixel[0], &pixel[2]);           /* bgr to rgb */
                }
            }
        }

        return true;
    }

    void read_tga_header(FILE* file, int* width, int* height, int* channels,
        int* is_rle, int* flip_h, int* flip_v) {
        const int TGA_HEADER_SIZE = 18;
        unsigned char header[TGA_HEADER_SIZE];
        int depth, idlength, imgtype, imgdesc;

        read_bytes(file, header, TGA_HEADER_SIZE);

        *width = header[12] | (header[13] << 8);
        *height = header[14] | (header[15] << 8);
        assert(*width > 0 && *height > 0);

        depth = header[16];
        assert(depth == 8 || depth == 24 || depth == 32);
        *channels = depth / 8;

        idlength = header[0];
        assert(idlength == 0);

        imgtype = header[2];
        assert(imgtype == 2 || imgtype == 3 || imgtype == 10 || imgtype == 11);
        *is_rle = imgtype == 10 || imgtype == 11;

        imgdesc = header[17];
        *flip_h = imgdesc & 0x10;
        *flip_v = imgdesc & 0x20;
    }

    void read_line(FILE* file, char line[LINE_SIZE]) {
        if (fgets(line, LINE_SIZE, file) == NULL) {
            assert(0);
        }
    }

    int starts_with(const char* string, const char* prefix) {
        return strncmp(string, prefix, strlen(prefix)) == 0;
    }

    unsigned char read_byte(FILE* file) {
        int byte = fgetc(file);
        assert(byte != EOF);
        return (unsigned char)byte;
    }
    void read_bytes(FILE* file, void* buffer, int size) {
        int count = (int)fread(buffer, 1, size, file);
        assert(count == size);
    }

    void write_bytes(FILE* file, void* buffer, int size) {
        int count = (int)fwrite(buffer, 1, size, file);
        assert(count == size);
    }
};
