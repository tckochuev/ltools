#pragma once

#include "VSExportFileAsImages.h"

#include "VSUtils.h"

class VSLeadtoolsManager :
  public tc::file_as_img::AbstractInterruptible<tc::file_as_img::fs::IExporter>,
  public tc::file_as_img::AbstractInterruptible<tc::file_as_img::mem::IExporter>,
  public tc::file_as_img::AbstractInterruptible<tc::file_as_img::fs::IThumbnailGenerator>,
  public tc::file_as_img::AbstractInterruptible<tc::file_as_img::mem::IThumbnailGenerator>
{
public:
    inline static const std::unordered_set<FileFormat> supportedFileFormats = {
        FileFormats::Ppt, FileFormats::Pptx, FileFormats::Odp
    };

    inline static const std::unordered_set<ImageFormat> supportedImageFormats = {
        ImageFormats::Jpg, ImageFormats::Png
    };

    inline static const std::unordered_set<PixelFormat> supportedPixelFormats = {
        PixelFormats::Rgba8888
    };

    VSLeadtoolsManager();

    void exportAsImages(
        const Path& file, const FileFormat& fileFormat,
        const PixelFormat& pixelFormat, const Any& options,
        const AnyBitmapConsumer& forEachImage
    ) override;

    void exportAsImages(
        const Path& file, const FileFormat& fileFormat,
        const Path& outputDir, const ImageFormat& imageFormat,
        const AnyImageNameGenerator& imageNameGenerator,
        const Any& options,
        const AnyImageNameConsumer& forEachImageName
    ) override;

    std::unique_ptr<IBitmap> generateThumbnail(
        const Path& file, const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
    ) override;

    String generateThumbnail(
        const Path& file, const FileFormat& fileFormat,
        const Path& outputDir, const ImageFormat& imageFormat,
        const AnyImageNameGenerator& imageNameGenerator,
        const Any& options
    ) override;

private:
    static void validateFileFormat(const FileFormat& fileFormat);
    static bool areOptionsValid(const Any& options);
    static void validateOptions(const Any& options);
    static void validateArgumentsFS(
        const FileFormat& fileFormat, const ImageFormat& imageFormat, const Any& options
    );
    static void validateArgumentsMem(
        const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
    );

#include "LEADTOOLS.lic.h"
#include "LEADTOOLS.lic.key.h"
    static inline bool licenseSet = false;
    static void setLicense();
};
