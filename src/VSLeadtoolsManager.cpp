#include "VSLeadtoolsManager.h"

using namespace tc::file_as_img;

VSLeadtoolsManager::VSLeadtoolsManager() :
    tc::file_as_img::AbstractInterruptible<tc::file_as_img::fs::IExporter>(
        std::make_unique<VSStdAtomicBoolInterruptor>()
    ),
    tc::file_as_img::AbstractInterruptible<tc::file_as_img::mem::IExporter>(
        std::make_unique<VSStdAtomicBoolInterruptor>()
    ),
    tc::file_as_img::AbstractInterruptible<tc::file_as_img::fs::IThumbnailGenerator>(
        std::make_unique<VSStdAtomicBoolInterruptor>()
    ),
    tc::file_as_img::AbstractInterruptible<tc::file_as_img::mem::IThumbnailGenerator>(
        std::make_unique<VSStdAtomicBoolInterruptor>()
    )
{
    if(!licenseSet) {
        setLicense();
        licenseSet = true;
    }
}

void VSLeadtoolsManager::validateFileFormat(const FileFormat& fileFormat)
{
    if(!supportedFileFormats.count(fileFormat)) {
        throw InvalidFileFormat();
    }
}

bool VSLeadtoolsManager::areOptionsValid(const Any& options)
{
    return !options.has_value();
}

void VSLeadtoolsManager::validateOptions(const Any& options) {
    if(!areOptionsValid(options)) {
        throw tc::err::exc::InvalidArgument("Invalid options");
    }
}

void VSLeadtoolsManager::validateArgumentsFS(
    const FileFormat& fileFormat, const ImageFormat& imageFormat, const Any& options
)
{
    validateFileFormat(fileFormat);
    if(!supportedImageFormats.count(imageFormat)) {
        throw InvalidImageFormat();
    }
    validateOptions(options);
}

void VSLeadtoolsManager::validateArgumentsMem(
    const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
)
{
    validateFileFormat(fileFormat);
    if(!supportedPixelFormats.count(pixelFormat)) {
        throw InvalidPixelFormat();
    }
    validateOptions(options);
}
