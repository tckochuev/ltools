#include <iostream>
#include <chrono>

#include "VSLeadtoolsManager.h"

int main(int argc, char* argv[])
{
    using namespace tc::file_as_img;
    try
    {
        if(argc != 5) {
            throw std::logic_error("Invalid arguments");
        }
        VSLeadtoolsManager exporter;
        Path inFile = argv[1];
        FileFormat inFmt = argv[2];
        Path outDir = argv[3];
        fs::ImageFormat outFmt = argv[4];
        fs::IncrementNameGenerator nameGen{1, std::string(".") + outFmt};
        auto tp1 = std::chrono::steady_clock::now();
        exporter.exportAsImages(inFile, inFmt, outDir, outFmt, nameGen, {}, [](auto&&) {});
        auto tp2 = std::chrono::steady_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::seconds>(tp2 - tp1).count() << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
