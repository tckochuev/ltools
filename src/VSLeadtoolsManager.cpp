#include "VSLeadtoolsManager.h"

#if defined(WIN32) || defined(__linux__)
	#include <boost/assign.hpp>
#endif

using namespace tc::file_as_img;
using namespace tc::file_as_img::fs;
using namespace tc::file_as_img::mem;

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

#if defined(WIN32) || defined(__linux__)

const tc::UnorderedBimap<FileFormat, decltype(FILE_PPTX)> VSLeadtoolsManager::fileFmtMap =
boost::assign::list_of<decltype(fileFmtMap)::relation>
(FileFormats::Pptx, FILE_PPTX)
(FileFormats::Ppt, FILE_PPT)
(FileFormats::Odp, FILE_ODP);

const tc::UnorderedBimap<ImageFormat, decltype(FILE_PNG)> VSLeadtoolsManager::imgFmtMap =
boost::assign::list_of<decltype(imgFmtMap)::relation>
(ImageFormats::Png, FILE_PNG)
(ImageFormats::Jpg, FILE_JPEG);

const std::unordered_map<ImageFormat, L_INT> VSLeadtoolsManager::imgFmtQFactorMap = {
	{ImageFormats::Png, 0},
	{ImageFormats::Jpg, 2}
};

void VSLeadtoolsManager::setLicense()
{
	call(L_SetLicenseBuffer, const_cast<L_UCHAR*>(LEADTOOLS_lic), LEADTOOLS_lic_len, const_cast<L_CHAR*>(LEADTOOLS_lic_key));
}

template<typename Interface, typename F>
void VSLeadtoolsManager::exportAsBitmaps(const Path& file, const FileFormat& fileFmt, F forEachBitmap)
{
	assert(supportedFileFormats.count(fileFmt));
	assert(fileFmtMap.left.count(fileFmt));

	auto checkInterrupt = [this] {Interface::checkInterrupt();};

	checkInterrupt();
	constexpr bool thumbnail = isThumbnailGenerator<Interface>;
	HBITMAPLIST bitmaps;
	auto uniqBitmaps = tc::makeUnique((pHBITMAPLIST)nullptr, [](pHBITMAPLIST bitmaps) {
		if(bitmaps) {
			call(L_DestroyBitmapList, *bitmaps);
		}
	});
	LOADFILEOPTION loadOpt{};
	call(L_GetDefaultLoadFileOption, &loadOpt, sizeof(LOADFILEOPTION));
	loadOpt.uStructSize = sizeof(LOADFILEOPTION);
	checkInterrupt();
	FILEINFO fileInfo;
	fileInfo.Flags = 0;
	fileInfo.uStructSize = sizeof(FILEINFO);
	if constexpr (thumbnail)
	{
		call(L_CreateBitmapList, &bitmaps);
		uniqBitmaps.reset(&bitmaps);
		checkInterrupt();
		loadOpt.PageNumber = 1;
		BITMAPHANDLE bitmap;
		call(L_LoadBitmap, const_cast<L_CHAR*>(file.string().c_str()), &bitmap, sizeof(BITMAPHANDLE),
			32, ORDER_RGB, &loadOpt, &fileInfo
		);
		auto uniqBitmap = tc::makeUnique(&bitmap, L_FreeBitmap);
		checkInterrupt();
		call(L_InsertBitmapListItem, bitmaps, 0, &bitmap);
		uniqBitmap.release();
		checkInterrupt();
	}
	else
	{
		call(L_LoadBitmapList, const_cast<L_CHAR*>(file.string().c_str()), &bitmaps, 32, ORDER_RGB, &loadOpt, &fileInfo);
		uniqBitmaps.reset(&bitmaps);
		checkInterrupt();
	}
	if(fileInfo.Format != fileFmtMap.left.at(fileFmt)) {
		throw std::runtime_error("unexpected format of loaded file");
	}
	L_UINT bitmapCount = 0;
	call(L_GetBitmapListCount, bitmaps, &bitmapCount);
	if(bitmapCount < 1) {
		throw NoPagesAvailable();
	}
	checkInterrupt();
	for(decltype(bitmapCount) i = 0; i < bitmapCount; ++i)
	{
		auto uniqBitmap = tc::makeUnique(new BITMAPHANDLE(), [](pBITMAPHANDLE bitmap) {
			if(bitmap) {
				L_FreeBitmap(bitmap);
				delete bitmap;
			}
		});
		call(L_RemoveBitmapListItem, bitmaps, 0, uniqBitmap.get());
		checkInterrupt();
		forEachBitmap(std::move(uniqBitmap));
	}
}

template<typename Interface>
std::unique_ptr<IBitmap> VSLeadtoolsManager::makeBitmap(BITMAPHANDLE& bitmap)
{
	assert(bitmap.BitsPerPixel == 32);
	assert(bitmap.Order == ORDER_RGB);
	assert(bitmap.Height > 0);
	assert(bitmap.BytesPerLine > 0);

	auto checkInterrupt = [this] {Interface::checkInterrupt();};
	struct BitmapDataReleaser {
		void operator()(pBITMAPHANDLE bitmap) const {
			assert(bitmap);
			if(L_ReleaseBitmap(bitmap) != SUCCESS) {
				//log error.
			}
		}
	};

	call(L_AccessBitmap, &bitmap);
	checkInterrupt();
	std::unique_ptr<BITMAPHANDLE, BitmapDataReleaser> dataReleaser(&bitmap);

	auto buffer = std::make_unique<mem::IBitmap::Byte[]>(bitmap.BytesPerLine * bitmap.Height);
	checkInterrupt();
	for(int i = 0; i < bitmap.Height; ++i) {
		const auto copiedCount = L_GetBitmapRow(
			&bitmap, (L_UCHAR*)(buffer.get() + i * bitmap.BytesPerLine), i, bitmap.BytesPerLine
		);
		if(copiedCount < 1) {
			const auto errorCode = copiedCount;
			throw LeadtoolsException(errorCode);
		}
		assert(copiedCount == bitmap.BytesPerLine);
		checkInterrupt();
	}

	assert(buffer);
	return std::make_unique<mem::Bitmap>(
		std::move(buffer), bitmap.Width, bitmap.Height, mem::PixelFormats::Rgba8888
	);
}

template<typename Interface>
String VSLeadtoolsManager::saveBitmap(
	BITMAPHANDLE& bitmap, const Path& outputDir, const ImageFormat& imageFormat,
	const AnyImageNameGenerator& imageNameGenerator
)
{
	assert(VSLeadtoolsManager::supportedImageFormats.count(imageFormat));
	assert(imgFmtMap.left.count(imageFormat));
	assert(imgFmtQFactorMap.count(imageFormat));

	auto checkInterrupt = [this] {Interface::checkInterrupt();};

	String imageName = imageNameGenerator();
	checkInterrupt();
	call(
		L_SaveBitmap,
		const_cast<L_CHAR*>((outputDir/imageName).string().c_str()),
		&bitmap,
		imgFmtMap.left.at(imageFormat),
		0,
		imgFmtQFactorMap.at(imageFormat),
		nullptr
	);
	checkInterrupt();
	return imageName;
}

void VSLeadtoolsManager::exportAsImages(
	const Path& file, const FileFormat& fileFormat,
	const PixelFormat& pixelFormat, const Any& options,
	const AnyBitmapConsumer& forEachImage
)
{
	using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::mem::IExporter>;
	validateArgumentsMem(fileFormat, pixelFormat, options);
	exportAsBitmaps<Interface>(file, fileFormat, [&](auto&& uniqBitmap) {
		forEachImage(makeBitmap<Interface>(*uniqBitmap));
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
	validateArgumentsFS(fileFormat, imageFormat, options);
	exportAsBitmaps<Interface>(file, fileFormat, [&](auto&& uniqBitmap) {
		forEachImageName(saveBitmap<Interface>(*uniqBitmap, outputDir, imageFormat, imageNameGenerator));
	});
}

auto VSLeadtoolsManager::generateThumbnail(
	const Path& file, const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
) -> std::unique_ptr<IBitmap>
{
	using Interface = tc::file_as_img::IInterruptible<tc::file_as_img::mem::IThumbnailGenerator>;
	validateArgumentsMem(fileFormat, pixelFormat, options);
	std::unique_ptr<IBitmap> img;
	exportAsBitmaps<Interface>(file, fileFormat, [&](auto&& uniqBitmap) {
		img = makeBitmap<Interface>(*uniqBitmap);
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
	validateArgumentsFS(fileFormat, imageFormat, options);
	String imageName;
	exportAsBitmaps<Interface>(file, fileFormat, [&](auto&& uniqBitmap) {
		imageName = saveBitmap<Interface>(*uniqBitmap, outputDir, imageFormat, imageNameGenerator);
	});
	return imageName;
}

#endif
