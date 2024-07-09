#include "VSLeadtoolsManager.h"

#include <cassert>
#include <memory>

#include <boost/assign.hpp>

#import <AVFoundation/AVFoundation.h>
#import <Leadtools.Codecs/LTRasterCodecs.h>
#import <Leadtools/LTRasterSupport.h>

#include "VSUtils.h"

namespace
{

using namespace tc::file_as_img;

struct ObjCDeleter {
  void operator()(id objCObject) const {
    [objCObject release];
  }
};

template<typename T>
using UniqueObjC = std::unique_ptr<T, ObjCDeleter>;

const tc::UnorderedBimap<FileFormat, LTRasterImageFormat> fileFmtMap =
  boost::assign::list_of<decltype(fileFmtMap)::relation>
  (FileFormats::Ppt, LTRasterImageFormatPpt)
  (FileFormats::Pptx, LTRasterImageFormatPptx)
  (FileFormats::Odp, LTRasterImageFormatOdp);

const tc::UnorderedBimap<FileFormat, LTRasterImageFormat> imgFmtMap =
  boost::assign::list_of<decltype(fileFmtMap)::relation>
  (fs::ImageFormats::Jpg, LTRasterImageFormatJpeg)
  (fs::ImageFormats::Png, LTRasterImageFormatPng);

auto throwErrorOr = [](NSError* error, const std::string& message) {
  if(error)
  {
    UniqueObjC<NSString> msg([error localizedDescription]);
    throw std::runtime_error([msg.get() UTF8String]);
  }
  else
  {
    throw std::runtime_error(message);
  }
};

UniqueObjC<NSString> nsStringFromPath(const Path& path) {
  UniqueObjC<NSString> res([NSString stringWithUTF8String:path.string().c_str()]);
  if(!res) {
    throw std::runtime_error("[NSString stringWithUTF8String] failed");
  }
  return res;
}

template<typename F>
void exportAsBitmaps(
  const Path& file, const FileFormat& fileFormat, bool thumbnail, const VSIInterruptible& interruptible, F forEachBitmap
)
{
  assert(VSLeadtoolsManager::supportedFileFormats.count(fileFormat));
  assert(fileFmtMap.left.count(fileFormat));

  auto checkInterrupt = [&interruptible] {interruptible.checkInterrupt();};

  checkInterrupt();
  UniqueObjC<LTRasterCodecs> codec([[LTRasterCodecs alloc] init]);
  if(!codec) {
    throw std::runtime_error("[[LTRasterCodecs alloc] init] failed");
  }
  checkInterrupt();
  const int lastPage = thumbnail ? 1 : -1;
  UniqueObjC<NSString> path(nsStringFromPath(file));
  checkInterrupt();
  NSError* error = nil;
  UniqueObjC<LTRasterImage> doc([codec.get() loadFile:path.get() bitsPerPixel:32 order:LTCodecsLoadByteOrderRgb
    firstPage:1 lastPage:lastPage error:&error]
  );
  if(!doc)
  {
    throwErrorOr(error, "LTRasterCodecs::loadFile failed");
  }
  if([doc.get() originalFormat] != fileFmtMap.left.at(fileFormat)) {
    throw std::runtime_error("unexpected format of loaded file");
  }
  checkInterrupt();
  auto pageCount = [doc.get() pageCount];
  if(pageCount < 1) {
    throw NoPagesAvailable();
  }
  checkInterrupt();
  for(int i = 1; i <= pageCount; ++i) {
    [doc.get() setPage:i];
    checkInterrupt();
    forEachBitmap(doc.get());
  }
}

std::unique_ptr<mem::IBitmap> makeBitmap(LTRasterImage* bitmap, const VSIInterruptible& interruptible)
{
  assert(bitmap);
  assert([bitmap bitsPerPixel] == 32);
  assert([bitmap order] == LTRasterByteOrderRgb);
  assert([bitmap height] > 0);
  assert([bitmap bytesPerLine] > 0);

  auto checkInterrupt = [&interruptible] {interruptible.checkInterrupt();};
  struct BitmapDataReleaser {
    void operator()(LTRasterImage* bitmap) const {
      NSError* error = nil;
      [bitmap releaseData:&error];
      if(error) {
        //Log error
      }
    }
  };

  NSError* error = nil;
  if(![bitmap accessData:&error])
  {
    throwErrorOr(error, "LTRasterImage::accessData failed");
  }
  checkInterrupt();
  std::unique_ptr<LTRasterImage, BitmapDataReleaser> dataReleaser(bitmap);

  auto buffer = std::make_unique<mem::IBitmap::Byte[]>([bitmap bytesPerLine] * [bitmap height]);
  checkInterrupt();
  for(int i = 0; i < [bitmap height]; ++i) {
    error = nil;
    auto copiedCount = [
      bitmap getRow:i
      buffer:(buffer.get() + i * [bitmap bytesPerLine])
      bufferCount:[bitmap bytesPerLine]
      error:&error
    ];
    if(error) {
      throwErrorOr(error, "LTRasterImage::getRow failed");
    }
    checkInterrupt();
    assert(copiedCount == [bitmap bytesPerLine]);
  }

  return std::make_unique<mem::Bitmap>(
    std::move(buffer), [bitmap width], [bitmap height], mem::PixelFormats::Rgba8888
  );
}

auto saveBitmap(
  LTRasterImage* bitmap,
  const Path& outputDir, const fs::ImageFormat& imageFormat,
  const fs::AnyImageNameGenerator& imageNameGenerator,
  const VSIInterruptible& interruptible
) -> fs::String
{
  assert(bitmap);
  assert(VSLeadtoolsManager::supportedImageFormats.count(imageFormat));
  assert(imgFmtMap.left.count(imageFormat));

  auto checkInterrupt = [&interruptible] {interruptible.checkInterrupt();};

  UniqueObjC<LTRasterCodecs> codec([[LTRasterCodecs alloc] init]);
  if(!codec) {
    throw std::runtime_error("[[LTRasterCodecs alloc] init] failed");
  }
  checkInterrupt();
  fs::String imageName = imageNameGenerator();
  checkInterrupt();
  UniqueObjC<NSString> path(nsStringFromPath(outputDir/imageName));
  NSError* error = nil;
  if(![codec.get() save:bitmap file:path.get() format:imgFmtMap.left.at(imageFormat) bitsPerPixel:0 error:&error])
  {
    throwErrorOr(error, "LTRasterodecs::save failed");
  }
  checkInterrupt();
  return imageName;
}

} //namespace

void VSLeadtoolsManager::exportAsImages(
  const Path& file, const FileFormat& fileFormat,
  const PixelFormat& pixelFormat, const Any& options,
  const AnyBitmapConsumer& forEachImage
)
{
  using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::mem::IExporter>;
  const VSIInterruptible& interruptible = static_cast<Interface&>(*this);
  validateArgumentsMem(fileFormat, pixelFormat, options);
  exportAsBitmaps(file, fileFormat, false, interruptible, [&](LTRasterImage* bitmap) {
    forEachImage(makeBitmap(bitmap, interruptible));
  });
}

void VSLeadtoolsManager::exportAsImages(
  const Path& file, const FileFormat& fileFormat,
  const Path& outputDir, const ImageFormat& imageFormat,
  const AnyImageNameGenerator& imageNameGenerator,
  const Any& options,
  const AnyImageNameConsumer& forEachImageName
)
{
  using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::fs::IExporter>;
  const VSIInterruptible& interruptible = static_cast<Interface&>(*this);
  validateArgumentsFS(fileFormat, imageFormat, options);
  exportAsBitmaps(file, fileFormat, false, interruptible, [&](LTRasterImage* bitmap) {
    forEachImageName(saveBitmap(bitmap, outputDir, imageFormat, imageNameGenerator, interruptible));
  });
}

auto VSLeadtoolsManager::generateThumbnail(
        const Path& file, const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
) -> std::unique_ptr<IBitmap>
{
  using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::mem::IThumbnailGenerator>;
  const VSIInterruptible& interruptible = static_cast<Interface&>(*this);
  validateArgumentsMem(fileFormat, pixelFormat, options);
  std::unique_ptr<IBitmap> img;
  exportAsBitmaps(file, fileFormat, true, interruptible, [&](LTRasterImage* bitmap) {
    img = makeBitmap(bitmap, interruptible);
  });
  return img;
}

auto VSLeadtoolsManager::generateThumbnail(
  const Path& file, const FileFormat& fileFormat,
  const Path& outputDir, const ImageFormat& imageFormat,
  const AnyImageNameGenerator& imageNameGenerator,
  const Any& options
) -> String
{
  using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::fs::IThumbnailGenerator>;
  const VSIInterruptible& interruptible = static_cast<Interface&>(*this);
  validateArgumentsFS(fileFormat, imageFormat, options);
  String imageName;
  exportAsBitmaps(file, fileFormat, true, interruptible, [&](LTRasterImage* bitmap) {
    imageName = saveBitmap(bitmap, outputDir, imageFormat, imageNameGenerator, interruptible);
  });
  return imageName;
}

void VSLeadtoolsManager::setLicense()
{
  NSData* lic = [NSData dataWithBytes:LEADTOOLS_lic length:LEADTOOLS_lic_len];
  assert(lic);
  NSData* keyData = [NSData dataWithBytes:LEADTOOLS_lic_key length:LEADTOOLS_lic_key_len];
  assert(keyData);
  UniqueObjC<NSString> key([[NSString alloc] initWithData:keyData encoding:NSUTF8StringEncoding]);
  if(!key) {
    throw std::runtime_error("[[NSString alloc] initWithData failed");
  }
  NSError* error = nil;
  if(![LTRasterSupport setLicenseData:lic developerKey:key.get() error:&error])
  {
    throwErrorOr(error, "LTRasterSupport::setLicenseData failed");
  }
}
