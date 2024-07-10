#pragma once

#include "VSExportFileAsImages.h"

#if defined(WIN32) || defined(__linux__)
#include "VSError.h"
#include <l_bitmap.h>
#include <lterr.h>
#include <ltfil.h>
#endif

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

#if defined(WIN32) || defined(__linux__)
	class LeadtoolsException : public tc::err::exc::ExceptionWithErrorCode<L_INT>
	{
	public:
		LeadtoolsException(L_INT code) :
			ExceptionWithErrorCode_(
				code, "Leadtools error: code: " + std::to_string(code) + " msg: " + makeErrorString(code)
			)
		{}

		static std::string makeErrorString(L_INT code) {
			constexpr size_t bufSize = 1024;
			char errBuf[bufSize] = {0};
			L_GetFriendlyErrorMessage(code, errBuf, bufSize, false);
			return errBuf;
		}
	};

	template<typename F, typename... Args>
	static auto call(F f, Args&&... args) {
		if(auto res = std::invoke(f, std::forward<Args>(args)...); res == SUCCESS) {
			return res;
		}
		else {
			throw LeadtoolsException(res);
		}
	}

	static const tc::UnorderedBimap<FileFormat, decltype(FILE_PPTX)> fileFmtMap;
	static const tc::UnorderedBimap<ImageFormat, decltype(FILE_PNG)> imgFmtMap;

	template<typename Interface, typename F>
	void exportAsBitmaps(const Path& file, const FileFormat& fileFmt, F forEachBitmap);

	template<typename Interface>
	std::unique_ptr<IBitmap> makeBitmap(BITMAPHANDLE& bitmap);

	template<typename Interface>
	String saveBitmap(BITMAPHANDLE& bitmap, const Path& outputDir, const ImageFormat& imageFormat,
		const AnyImageNameGenerator& imageNameGenerator
	);

#endif
};
