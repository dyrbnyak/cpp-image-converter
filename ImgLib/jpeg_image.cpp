#include "ppm_image.h"

#include <array>
#include <fstream>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// === Константы JPEG формата ===
namespace jpeg_constants {
    // Параметры цвета
    constexpr int COMPONENTS_RGB = 3;           // 3 компонента (R,G,B)
    
    // Каналы в порядке RGB
    constexpr int CHANNEL_R = 0;
    constexpr int CHANNEL_G = 1;
    constexpr int CHANNEL_B = 2;
    
    // Альфа-канал (для заполнения)
    constexpr unsigned char ALPHA_OPAQUE = 255;
}

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// В эту функцию вставлен код примера из библиотеки libjpeg.
// Измените его, чтобы адаптировать к переменным file и image.
// Задание качества уберите - будет использовано качество по умолчанию
bool SaveJPEG(const Path& file, const Image& image) {
    using namespace jpeg_constants;
    
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    
    FILE* outfile;
    JSAMPROW row_pointer[1];
    int row_stride;
    
    // Открытие файла с учётом Visual Studio
#ifdef _MSC_VER
    if ((outfile = _wfopen(file.wstring().c_str(), L"wb")) == NULL) {
        return false;
    }
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
        return false;
    }
#endif
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);
    
    // Берём размеры из параметра image
    cinfo.image_width = image.GetWidth();
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = COMPONENTS_RGB;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    // jpeg_set_quality удалён
    
    jpeg_start_compress(&cinfo, TRUE);
    
    row_stride = image.GetWidth() * COMPONENTS_RGB;
    
    // Подготовка буфера для одной строки
    std::vector<unsigned char> buffer(row_stride);
    
    while (cinfo.next_scanline < cinfo.image_height) {
        int y = cinfo.next_scanline;
        const Color* line = image.GetLine(y);
        
        // Конвертируем строку изображения в формат RGB
        for (int x = 0; x < image.GetWidth(); ++x) {
            buffer[x * COMPONENTS_RGB + CHANNEL_R] = static_cast<unsigned char>(line[x].r);
            buffer[x * COMPONENTS_RGB + CHANNEL_G] = static_cast<unsigned char>(line[x].g);
            buffer[x * COMPONENTS_RGB + CHANNEL_B] = static_cast<unsigned char>(line[x].b);
        }
        
        row_pointer[0] = buffer.data();
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
    
    return true;
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    using namespace jpeg_constants;
    
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * COMPONENTS_RGB;
        line[x] = Color{byte{pixel[CHANNEL_R]}, 
                        byte{pixel[CHANNEL_G]}, 
                        byte{pixel[CHANNEL_B]}, 
                        byte{ALPHA_OPAQUE}};
    }
}

Image LoadJPEG(const Path& file) {
    using namespace jpeg_constants;
    
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;

    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), L"rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = COMPONENTS_RGB;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;

    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveSсanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib