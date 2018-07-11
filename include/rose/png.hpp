#ifndef ROSE_PNG_HPP
#define ROSE_PNG_HPP

#include <png.h>
#include <rose/bitmap.hpp>
#include <rose/result.hpp>

namespace rose {

  struct Png {

    enum class Error {
      FILE_OPEN_ERR,
      PNG_INTERNAL_ERR,
      PNG_LOAD_ERR,
      PNG_WRITE_ERR,
      INVALID_FORMAT_ERR
    };

    template<bitmap::Fmt FMT>
    static Result<Bitmap<FMT>, Error> load(const char* filename) noexcept {
      FILE* file = fopen(filename, "rb");
      if (!file) {
        return Result<Bitmap<FMT>, Error>::err(Error::FILE_OPEN_ERR);
      }

      png_structp reader = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                  nullptr, nullptr, nullptr);
      if (!reader) {
        fclose(file);
        return Result<Bitmap<FMT>, Error>::err(Error::PNG_INTERNAL_ERR);
      }

      png_infop info = png_create_info_struct(reader);
      if (!info) {
        png_destroy_read_struct(&reader, nullptr, nullptr);
        fclose(file);
        return Result<Bitmap<FMT>, Error>::err(Error::PNG_INTERNAL_ERR);
      }

      png_bytep* rows = nullptr;
      uint8_t* data = nullptr;

      if (setjmp(png_jmpbuf(reader))) {
        // Jump here on error
        png_destroy_read_struct(&reader, &info, nullptr);
        if (rows != nullptr) delete[] rows;
        if (data != nullptr) delete[] data;
        fclose(file);
        return Result<Bitmap<FMT>, Error>::err(Error::PNG_LOAD_ERR);
      }

      png_init_io(reader, file);
      png_read_info(reader, info);

      png_int_32 color_type = png_get_color_type(reader, info);
      size_t width = png_get_image_width(reader, info);
      size_t height = png_get_image_height(reader, info);

      png_set_strip_16(reader);

      if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(reader);
      }

      if (color_type == PNG_COLOR_TYPE_GRAY ||
          color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(reader);
      }

      if (png_get_valid(reader, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(reader);
      }

      if (FMT == bitmap::Fmt::RGBA) {
        png_set_filler(reader, 0xff, PNG_FILLER_AFTER);
      }

      if (FMT == bitmap::Fmt::RGB) {
        png_color_16 white;
        white.red = 255;
        white.green = 255;
        white.blue = 255;

        png_set_background(reader, &white, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
      }

      png_read_update_info(reader, info);
      png_uint_32 rowbytes = png_get_rowbytes(reader, info);

      // Allocate memory for bitmap data and row pointers
      rows = new png_bytep[height];
      data = new uint8_t[height * rowbytes];

      // Make row pointers point to correct index in the data
      for (size_t i = 0; i < height; ++i) {
        // Reverse row order as PNG is top to bottom
        // while we expect bottom to top
        png_uint_32 row = (height - i - 1) * rowbytes;
        rows[i] = (png_bytep) (data + row);
      }

      png_read_image(reader, rows);

      png_destroy_read_struct(&reader, &info, nullptr);
      delete[] rows;
      fclose(file);

      return Result<Bitmap<FMT>, Error>::ok({width, height, data});
    }

    template<bitmap::Fmt FMT>
    static void free(const Bitmap<FMT>& bitmap) noexcept {
      delete[] bitmap.d;
    }

    template<bitmap::Fmt FMT>
    static Result<void, Error> write(const char* filename,
                                     const Bitmap<FMT>& bitmap) noexcept {
      FILE *file = fopen(filename, "wb");
      if (!file) {
        return Result<void, Error>::err(Error::FILE_OPEN_ERR);
      }

      png_structp writer = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                   nullptr, nullptr, nullptr);
      if (!writer) {
        fclose(file);
        return Result<void, Error>::err(Error::PNG_INTERNAL_ERR);
      }

      png_infop info = png_create_info_struct(writer);
      if (!info) {
        png_destroy_write_struct(&writer, nullptr);
        fclose(file);
        return Result<void, Error>::err(Error::PNG_INTERNAL_ERR);
      }

      png_bytep* rows = nullptr;

      if (setjmp(png_jmpbuf(writer))) {
        // Jump here on error
        png_destroy_write_struct(&writer, &info);
        if (rows != NULL) delete[] rows;
        fclose(file);
        return Result<void, Error>::err(Error::PNG_WRITE_ERR);
      }

      png_init_io(writer, file);

      png_int_32 color_type;
      if (bitmap.format() == bitmap::Fmt::RGB) {
        color_type = PNG_COLOR_TYPE_RGB;
      } else if (bitmap.format() == bitmap::Fmt::RGBA) {
        color_type = PNG_COLOR_TYPE_RGBA;
      } else {
        return Result<void, Error>::err(Error::INVALID_FORMAT_ERR);
      }

      png_set_IHDR(writer, info, bitmap.width(), bitmap.height(),
                   8, color_type, PNG_INTERLACE_NONE,
                   PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

      png_write_info(writer, info);

      rows = new png_bytep[bitmap.height()];

      size_t rowbytes = png_get_rowbytes(writer, info);
      for (size_t i = 0; i < bitmap.height(); ++i) {
        // Reverse row order as PNG is top to bottom
        // while the Bitmap is bottom to top
        png_uint_32 row = (bitmap.height() - i - 1) * rowbytes;
        rows[i] = (png_bytep) (bitmap.data() + row);
      }

      png_write_image(writer, rows);
      png_write_end(writer, nullptr);

      png_destroy_write_struct(&writer, &info);
      delete[] rows;
      fclose(file);

      return Result<void, Error>::ok();
    }

  };

}

#endif
