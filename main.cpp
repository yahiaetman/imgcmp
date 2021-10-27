#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <flags/flags.h>

const char* HELP = 
    "imgcmp: a simple pixel-wise image comparator\n"
    "\n"
    "Usage:\n"
    "   imgcmp [options] <path-to-1st-image> <path-to-2nd-image>\n"
    "\n"
    "This tool compares between two images pixel by pixel.\n"
    "For each pixel, the channels are compared with their counterparts.\n"
    "If the value error for any channel exceeds the threshold, the whole pixel is considered different.\n"
    "If the number of different pixels exceeds the specified limit, the result is a mismatch.\n"
    "The exit code will be 0 if the images match and -1 if they don't.\n"
    "When generating an error image, channels that don't pass the threshold will be kept 0.\n"
    "Otherwise the channel's value will be 128 (half intensity) plus half the error value.\n"
    "\n"
    "Options:\n"
    "   -s, --silent\t\t\t= Run in silent mode. No console output will be generated. Default: false.\n"
    "   -v, --verbose\t\t= Run in verbose mode. Extra console output will be generated. Default: false.\n"
    "   -o <path-to-error-image>\t= Outputs the pixel error into a png image at the given path.\n"
    "   -t <pixel-threshold>\t\t= Sets a threshold [0-1] on the maximum allowed per-channel error. Default: 0.\n"
    "   \t\t\t\t  if 0, any difference passes the threshold. if 1, nothing passes the threshold.\n"
    "   -e <count>\t\t\t= Sets the number of pixels allowed to be different before the result is considered a mismatch. Default: 0.\n"
    "   -e <percent>%\t\t= Sets the percentage of pixels allowed to be different before the result is considered a mismatch.\n"
    "   -h, --help\t\t\t= Prints usage info then exits.\n";

union U8Color {
    stbi_uc channels[3];
    struct {
        stbi_uc r, b, g;
    };
};

int main(int argc, char** argv) {
    flags::args args(argc, argv);

    bool verbose = args.get<bool>("v", false) || args.get<bool>("verbose", false);
    bool silent = args.get<bool>("s", false) || args.get<bool>("silent", false);

    if(args.get<bool>("h", false) || args.get<bool>("help", false)){
        std::cout << HELP << std::endl;
        return 0;
    }
    
    auto path1 = args.get<std::string>(0);
    auto path2 = args.get<std::string>(1);

    if(!path1.has_value() || !path2.has_value()){
        std::cout << "You must specify the paths to the two images that will be compared.\n\n";
        std::cout << HELP << std::endl;
        return -1;
    }

    int w1, h1, c1;
    U8Color* img1 = (U8Color*)stbi_load((*path1).c_str(), &w1, &h1, &c1, 3);
    if(!img1){
        if(!silent) std::cerr << "Failed to open " << *path1 << std::endl;
        return -1;
    }

    int w2, h2, c2;
    U8Color* img2 = (U8Color*)stbi_load((*path2).c_str(), &w2, &h2, &c2, 3);
    if(!img2){
        if(!silent) std::cerr << "Failed to open " << *path2 << std::endl;
        free(img1);
        return -1;
    }

    if(w1 != w2 || h1 != h2){
        if(!silent) std::cout << "Images have different sizes" << std::endl;
        free(img1); free(img2);
        return -1;
    }

    int pixel_error_threshold = int(255 * args.get<float>("t", 0.0));

    int size = w1 * h1;
    int image_error_threshold = 0;
    if(auto option = args.get<std::string>("e"); option.has_value()){
        auto& str = *option;
        if(str[str.size()-1] == '%') image_error_threshold = int((w1*h1)*(atof(str.c_str()))/100);
        else image_error_threshold = atoi(str.c_str());
    }

    U8Color* diff = (U8Color*)malloc(size * sizeof(U8Color));

    int wrong_pixels = 0;
    for(int pixel = 0; pixel < size; ++pixel){
        bool wrong_pixel = false;
        U8Color& color1 = img1[pixel];
        U8Color& color2 = img2[pixel];
        U8Color& pdiff = diff[pixel];
        for(int channel = 0; channel < 3; ++channel){
            stbi_uc error = (stbi_uc)abs(int(color1.channels[channel]) - int(color2.channels[channel]));
            if(error > pixel_error_threshold){
                pdiff.channels[channel] = 128 | (error >> 1);
                wrong_pixel = true;
            } else {
                pdiff.channels[channel] = 0;
            }
        }
        wrong_pixels += wrong_pixel;    
    }

    auto out_path = args.get<std::string>("o");

    if(out_path.has_value()){
        stbi_write_png((*out_path).c_str(), w1, h1, 3, diff, 3*w1);
    }

    int return_value = 0;
    if(wrong_pixels > image_error_threshold){
        return_value = -1;
    }

    if(!silent){
        std::cout << (return_value == 0 ? "MATCH" : "MISMATCH DETECTED") << std::endl;
        if(verbose){
            std::cout << "Different Pixels: " << float(100*wrong_pixels)/(w1*h1) << "%" << std::endl;
        }
    }

    free(img1); free(img2); free(diff);

    return return_value;
}
