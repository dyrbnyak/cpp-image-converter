#include "ppm_image.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

namespace ppm_constants {
    // Сигнатура формата
    constexpr std::string_view SIGNATURE = "P6"sv;
    
    // Параметры цвета
    static constexpr int MAX_COLOR_VALUE = 255;        // максимальное значение цвета (8 бит)
    static constexpr int BYTES_PER_PIXEL = 3;          // 3 байта на пиксель (R,G,B)
    
    // Каналы цвета (порядок в PPM - RGB)
    constexpr int CHANNEL_R = 0;                // смещение Red
    constexpr int CHANNEL_G = 1;                // смещение Green
    constexpr int CHANNEL_B = 2;                // смещение Blue
}

bool SavePPM(const Path& file, const Image& image) {
    using namespace ppm_constants;
    
    ofstream out(file, ios::binary);
    if (!out) return false;

    // Записываем заголовок PPM
    out << SIGNATURE << '\n' 
        << image.GetWidth() << ' ' << image.GetHeight() << '\n' 
        << MAX_COLOR_VALUE << '\n';

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    
    // Буфер для одной строки
    const size_t rowSize = static_cast<size_t>(w) * BYTES_PER_PIXEL;
    std::vector<char> buffer(rowSize);

    for (int y = 0; y < h; ++y) {
        const Color* line = image.GetLine(y);
        
        // Заполняем буфер в формате RGB
        for (int x = 0; x < w; ++x) {
            buffer[x * BYTES_PER_PIXEL + CHANNEL_R] = static_cast<char>(line[x].r);
            buffer[x * BYTES_PER_PIXEL + CHANNEL_G] = static_cast<char>(line[x].g);
            buffer[x * BYTES_PER_PIXEL + CHANNEL_B] = static_cast<char>(line[x].b);
        }
        
        out.write(buffer.data(), rowSize);
        if (!out) return false;
    }

    return out.good();
}

Image LoadPPM(const Path& file) {
    using namespace ppm_constants;
    
    // Открываем поток с флагом ios::binary
    ifstream ifs(file, ios::binary);
    if (!ifs) return {};
    
    std::string sign;
    int w, h, color_max;

    // Читаем заголовок: формат, размеры, максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    // Проверяем формат: только P6 с максимальным значением 255
    if (sign != SIGNATURE || color_max != MAX_COLOR_VALUE) {
        return {};
    }

    // Пропускаем один байт - это конец строки после заголовка
    const char next = ifs.get();
    if (next != '\n') {
        return {};
    }

    // Создаем изображение
    Image result(w, h, Color::Black());
    if (!result) return {};
    
    // Буфер для одной строки
    const size_t rowSize = static_cast<size_t>(w) * BYTES_PER_PIXEL;
    std::vector<char> buffer(rowSize);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        
        ifs.read(buffer.data(), rowSize);
        if (!ifs) return {};
        
        // Преобразуем из RGB в Color
        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buffer[x * BYTES_PER_PIXEL + CHANNEL_R]);
            line[x].g = static_cast<byte>(buffer[x * BYTES_PER_PIXEL + CHANNEL_G]);
            line[x].b = static_cast<byte>(buffer[x * BYTES_PER_PIXEL + CHANNEL_B]);
        }
    }

    return result;
}

}  // namespace img_lib