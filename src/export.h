#pragma once

#include <filesystem>

namespace tc
{

using Path = std::filesystem::path;
using FileFormat = std::string;
using ImageFormat = std::string;

void exportAsImages(const Path& inFile, const FileFormat& inFmt, const Path& outDir, const ImageFormat& outFmt, bool thumbnail);
void generateThumbnail(const Path& inFile, const FileFormat& inFmt, const Path& outDir, const ImageFormat& outFmt);

}
