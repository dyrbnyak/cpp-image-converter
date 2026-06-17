#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <bmp_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;

enum Format{
    JPEG
    ,PPM
    ,BMP
    ,UNKNOWN
};


namespace format_image{
class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
}; 


class PPM : public ImageFormatInterface{
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override{
        return img_lib::SavePPM(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override{
        return img_lib::LoadPPM(file);
    }
};


class JPEG : public ImageFormatInterface{
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override{
        return img_lib::SaveJPEG(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override{
        return img_lib::LoadJPEG(file);
    }
}; 

class BMP : public ImageFormatInterface{
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override{
        return img_lib::SaveBMP(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override{
        return img_lib::LoadBMP(file);
    }
}; 

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }

    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    if(ext == ".bmp"sv) {
        return Format::BMP;
    }

    return Format::UNKNOWN;
}

ImageFormatInterface* GetFormatInterface(const img_lib::Path& path){
    Format format = GetFormatByExtension(path);

    if(format == Format::JPEG){
        return new JPEG();

    } else if (format == Format::PPM){
        return new PPM();

    } else if (format == Format::BMP){
        return new BMP();

    } else if (format == Format::UNKNOWN){
        return nullptr;
    }
} 
} // format_image










int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    format_image::ImageFormatInterface* in_format = format_image::GetFormatInterface(in_path);
    if(!in_format){
        cerr << "Unknown format of the input file"sv << endl;
        return 2;
    }

    img_lib::Image image = in_format -> LoadImage(in_path);

    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    format_image::ImageFormatInterface* out_format = format_image::GetFormatInterface(out_path);
    if (!out_format) {
        cerr << "Unknown format of the output file"sv << endl;
        return 3;
    }

    bool success = out_format->SaveImage(out_path, image);
    delete in_format; // Освобождаем память
    delete out_format;

    if (success) {
        cout << "Successfully converted"sv << endl;
    } else {
        // Здесь можно добавить обработку других ошибок записи файла
        cerr << "Error saving the file"sv << endl;
        return -1;
    }
}