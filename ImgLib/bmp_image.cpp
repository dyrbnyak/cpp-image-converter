#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;



namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bfType;          // подпись "BM" 
    uint32_t bfSize;          // общий размер файла
    uint32_t bfReserved;      // зарезервировано (0)
    uint32_t bfOffBits;       // смещение до пиксельных данных
} PACKED_STRUCT_END


PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t biSize;          // размер этого заголовка = 40
    int32_t  biWidth;         // ширина в пикселях
    int32_t  biHeight;        // высота в пикселях
    uint16_t biPlanes;        // количество плоскостей = 1
    uint16_t biBitCount;      // бит на пиксель = 24
    uint32_t biCompression;   // тип сжатия = 0 (BI_RGB)
    uint32_t biSizeImage;     // размер области данных
    int32_t  biXPelsPerMeter; // горизонтальное разрешение = 11811
    int32_t  biYPelsPerMeter; // вертикальное разрешение = 11811
    uint32_t biClrUsed;       // используемых цветов = 0
    uint32_t biClrImportant;  // значимых цветов = 0x1000000
} PACKED_STRUCT_END


namespace bmp_constants {
    // Сигнатура
    constexpr uint16_t SIGNATURE = 'B' | ('M' << 8);// Заголовок, всегда BM
    
    // Размеры заголовков
    constexpr size_t FILE_HEADER_SIZE = sizeof(BitmapFileHeader);   // 14 байт
    constexpr size_t INFO_HEADER_SIZE = sizeof(BitmapInfoHeader);   // 40 байт
    constexpr size_t HEADERS_TOTAL_SIZE = FILE_HEADER_SIZE + INFO_HEADER_SIZE;
    
    // Параметры InfoHeader
    constexpr uint16_t PLANES = 1;                    // количество плоскостей
    constexpr uint16_t BITS_PER_PIXEL = 24;           // бит на пиксель (24 = RGB по 8 бит)
    constexpr uint32_t COMPRESSION_NONE = 0;          // BI_RGB (без сжатия)
    constexpr int32_t DPI_11811 = 11811;              // разрешение ~300 DPI (11811 пикселей на метр)
    constexpr uint32_t CLR_USED_NONE = 0;             // не используется
    constexpr uint32_t CLR_IMPORTANT_ALL = 0x1000000; // все цвета важны
    
    // Выравнивание строк
    constexpr int BYTES_PER_PIXEL = 3;                // 3 байта на пиксель (R,G,B)
    constexpr int STRIDE_ALIGNMENT = 4;               // выравнивание до 4 байт
    
    // Размеры каналов
    constexpr int CHANNEL_R = 2;                      // смещение Red в BGR
    constexpr int CHANNEL_G = 1;                      // смещение Green в BGR
    constexpr int CHANNEL_B = 0;                      // смещение Blue в BGR
} // bmp_constants


// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    using namespace bmp_constants;
    return STRIDE_ALIGNMENT * ((w * BYTES_PER_PIXEL + (STRIDE_ALIGNMENT - 1)) / STRIDE_ALIGNMENT);
}

static size_t CalculatePixelDataSize(int w, int h) {
    return static_cast<size_t>(GetBMPStride(w)) * h;
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    using namespace bmp_constants;
    
    if (!image) return false;
    
    int w = image.GetWidth();
    int h = image.GetHeight();
    int stride = GetBMPStride(w);
    size_t pixelDataSize = CalculatePixelDataSize(w, h);
    
    // Заполняем BitmapFileHeader
    BitmapFileHeader fileHeader{};

    fileHeader.bfType = SIGNATURE;
    fileHeader.bfSize = HEADERS_TOTAL_SIZE + pixelDataSize;
    fileHeader.bfReserved = 0;
    fileHeader.bfOffBits = HEADERS_TOTAL_SIZE;
    
    // Заполняем BitmapInfoHeader
    BitmapInfoHeader infoHeader{};

    infoHeader.biSize = INFO_HEADER_SIZE;
    infoHeader.biWidth = w;
    infoHeader.biHeight = h;
    infoHeader.biPlanes = PLANES;
    infoHeader.biBitCount = BITS_PER_PIXEL;
    infoHeader.biCompression = COMPRESSION_NONE;
    infoHeader.biSizeImage = pixelDataSize;
    infoHeader.biXPelsPerMeter = DPI_11811;
    infoHeader.biYPelsPerMeter = DPI_11811;
    infoHeader.biClrUsed = CLR_USED_NONE;
    infoHeader.biClrImportant = CLR_IMPORTANT_ALL;
    
    // Открываем файл для записи (бинарный режим)
    std::ofstream out(file, std::ios::binary);
    if (!out) return false;
    
    // Записываем заголовки
    out.write(reinterpret_cast<const char*>(&fileHeader), FILE_HEADER_SIZE);
    out.write(reinterpret_cast<const char*>(&infoHeader), INFO_HEADER_SIZE);
    
    // Записываем пиксельные данные (строки снизу вверх)
    std::vector<char> rowBuffer(stride, 0);
    
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        
        // Заполняем буфер в формате BGR
        for (int x = 0; x < w; ++x) {
            rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_B] = static_cast<char>(line[x].b);
            rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_G] = static_cast<char>(line[x].g);
            rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_R] = static_cast<char>(line[x].r);
        }
        
        out.write(rowBuffer.data(), stride);
        if (!out) return false;
    }
    
    return true;
}


// напишите эту функцию
Image LoadBMP(const Path& file) {
    using namespace bmp_constants;
    
    std::ifstream in(file, std::ios::binary);
    if (!in) return {};
    
    BitmapFileHeader fileHeader{};
    BitmapInfoHeader infoHeader{};
    
    in.read(reinterpret_cast<char*>(&fileHeader), FILE_HEADER_SIZE);
    in.read(reinterpret_cast<char*>(&infoHeader), INFO_HEADER_SIZE);
    
    // Проверяем, что это BMP и формат нам подходит
    if (fileHeader.bfType != SIGNATURE) return {};
    if (infoHeader.biBitCount != BITS_PER_PIXEL) return {};
    if (infoHeader.biCompression != COMPRESSION_NONE) return {};
    
    int w = infoHeader.biWidth;
    int h = abs(infoHeader.biHeight);
    int stride = GetBMPStride(w);
    
    // Отрицательная высота означает порядок строк сверху вниз
    bool topDown = infoHeader.biHeight < 0;
    
    Image result(w, h, Color::Black());
    if (!result) return {};
    
    // Позиционируемся на начало пиксельных данных
    // Нельзя просто последовательно читать, так как размер данные может быть разный и мы попадем не туда
    // Надежнее "прыгнуть" сразу куда надо
    in.seekg(fileHeader.bfOffBits, std::ios::beg);
    
    std::vector<char> rowBuffer(stride);
    
    for (int y = 0; y < h; ++y) {
        int fileY = topDown ? y : (h - 1 - y);
        
        in.read(rowBuffer.data(), stride);
        if (!in) return {};
        
        Color* line = result.GetLine(fileY);
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_B]);
            line[x].g = static_cast<byte>(rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_G]);
            line[x].r = static_cast<byte>(rowBuffer[x * BYTES_PER_PIXEL + CHANNEL_R]);
        }
    }
    
    return result;
}

}  // namespace img_lib


