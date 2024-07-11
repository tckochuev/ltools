#pragma once

#include <any>
#include <memory>
#include <type_traits>
#include <filesystem>

#include <boost/lexical_cast.hpp>

#include "VSUtils.h"
#include "VSIInterruptible.h"

namespace tc::file_as_img
{

class InvalidFileFormat : public tc::err::exc::InvalidArgument
{
public:
	InvalidFileFormat() : tc::err::exc::InvalidArgument("Invalid format of file") {}
	using tc::err::exc::InvalidArgument::InvalidArgument;
};

///@brief Indicates that input file is a valid multipage file of input format,
/// but there is 0 pages in it.
class NoPagesAvailable : public std::runtime_error
{
public:
	NoPagesAvailable() : std::runtime_error("No pages available in file") {}
	using std::runtime_error::runtime_error;
};

using Any = std::any;
using Path = std::filesystem::path;
using FileFormat = std::string;
struct FileFormats
{
	FileFormats() = delete;
	inline static const char* Ppt = "ppt";
	inline static const char* Pptx = "pptx";
	inline static const char* Pdf = "pdf";
	inline static const char* Odp = "odp";
};

struct TypesHolder
{
	using Any = tc::file_as_img::Any;
	using Path = tc::file_as_img::Path;
	using FileFormat = tc::file_as_img::FileFormat;
	using InvalidFileFormat = tc::file_as_img::InvalidFileFormat;
	using NoPagesAvailable = tc::file_as_img::NoPagesAvailable;
	using FileFormats = tc::file_as_img::FileFormats;
};

namespace fs
{

class InvalidImageFormat : public tc::err::exc::InvalidArgument
{
public:
	InvalidImageFormat() : tc::err::exc::InvalidArgument("Invalid format of image") {}
	using tc::err::exc::InvalidArgument::InvalidArgument;
};

using ImageFormat = std::string;
using String = std::string;
using AnyImageNameGenerator = std::function<String()>;
struct ImageFormats
{
	inline static const char* Jpg = "jpg";
	inline static const char* Png = "png";
};

struct TypesHolder : tc::file_as_img::TypesHolder
{
	using ImageFormat = fs::ImageFormat;
	using String = fs::String;
	using AnyImageNameGenerator = fs::AnyImageNameGenerator;
	using InvalidImageFormat = fs::InvalidImageFormat;
	using ImageFormats = fs::ImageFormats;
};

class IThumbnailGenerator : public TypesHolder
{
	INTERFACE(IThumbnailGenerator)
public:
	virtual String generateThumbnail(
		const Path& file, const FileFormat& fileFormat,
		const Path& outputDir, const ImageFormat& imageFormat,
		const AnyImageNameGenerator& imageNameGenerator,
		const Any& options
	) = 0;
};

class IExporter : public TypesHolder
{
	INTERFACE(IExporter)
public:
	using AnyImageNameConsumer = std::function<void(const String&)>;
	///@brief Exports @p file of @p fileFormat as images of @p imageFormat to @p outputDir,
	/// invokes @p forEachImageName on all names produced with @p imageNameGenerator.
	virtual void exportAsImages(
		const Path& file, const FileFormat& fileFormat,
		const Path& outputDir, const ImageFormat& imageFormat,
		const AnyImageNameGenerator& imageNameGenerator,
		const Any& options,
		const AnyImageNameConsumer& forEachImageName
	) = 0;
};

template<typename Value = int>
struct IncrementNameGenerator
{
	using String = TypesHolder::String;

	IncrementNameGenerator() = default;
	IncrementNameGenerator(Value value, String postfix)
		: value(std::move(value)), postfix(std::move(postfix))
	{}

	String operator()() {
		return boost::lexical_cast<String>(value++) + postfix;
	}

	Value value{};
	String postfix{};
};

} //namespace fs

namespace mem
{

class InvalidPixelFormat : public tc::err::exc::InvalidArgument
{
public:
	InvalidPixelFormat() : tc::err::exc::InvalidArgument("Invalid pixel format") {}
	using tc::err::exc::InvalidArgument::InvalidArgument;
};

class IBitmap;
using PixelFormat = std::string;
struct PixelFormats
{
	///@brief specifies one 32-bit unit encoding(0xAARRGGBB).
	inline static const char* Argb32 = "argb32";
	///@brief specifies 4 8-bit units in order r,g,b,a.
	inline static const char* Rgba8888 = "rgba8888";
};

struct TypesHolder : tc::file_as_img::TypesHolder
{
	using PixelFormat = mem::PixelFormat;
	using IBitmap = mem::IBitmap;
	using InvalidPixelFormat = mem::InvalidPixelFormat;
	using PixelFormats = mem::PixelFormats;
};

class IBitmap
{
	INTERFACE(IBitmap)
public:
	using Size = size_t;
	using Byte = std::byte;
	using PixelFormat = TypesHolder::PixelFormat;

	virtual Size width() const = 0;
	virtual Size height() const = 0;
	virtual PixelFormat pixelFormat() const = 0;
	virtual const Byte* data() const = 0;
};

class Bitmap : public IBitmap
{
public:
	Bitmap(std::shared_ptr<const Byte[]> data, Size width, Size height, PixelFormat format)
		: m_format(std::move(format)), m_data(std::move(data)), m_width(width), m_height(height)
	{
		assert(m_data);
		assert(width > 0);
		assert(height > 0);
	}
	Bitmap(Bitmap&&) noexcept = delete;

	Size width() const override {
		return m_width;
	}
	Size height() const override {
		return m_height;
	}
	PixelFormat pixelFormat() const override {
		return m_format;
	}
	const Byte* data() const override {
		return m_data.get();
	}

private:
	PixelFormat m_format;
	std::shared_ptr<const Byte[]> m_data;
	Size m_width;
	Size m_height;
};

class IThumbnailGenerator : public TypesHolder
{
	INTERFACE(IThumbnailGenerator)
public:
	virtual std::unique_ptr<IBitmap> generateThumbnail(
		const Path& file, const FileFormat& fileFormat, const PixelFormat& pixelFormat, const Any& options
	) = 0;
};

class IExporter : public TypesHolder
{
	INTERFACE(IExporter)
public:
	using AnyBitmapConsumer = std::function<void(std::unique_ptr<IBitmap>)>;
	///@brief Exports @p file of @p fileFormat as instances of type, derived from IImage, that model images of
	/// @p pixelFormat and invokes @p forEachImage on all produced instances.
	virtual void exportAsImages(
		const Path& file, const FileFormat& fileFormat,
		const PixelFormat& pixelFormat, const Any& options,
		const AnyBitmapConsumer& forEachBitmap
	) = 0;
};

} //namespace mem

template<typename Interface>
struct TaskOf
{};

template<typename Interface>
inline constexpr TaskOf<Interface> taskOf{};

template<typename Interface>
class IInterruptible : public VSIInterruptible, public Interface
{
	INTERFACE(IInterruptible)
public:
	virtual bool getInterruptionFlagFor(TaskOf<Interface>) const = 0;
	virtual void setInterruptionFlagFor(TaskOf<Interface>, bool interrupt) = 0;
	bool getInterruptionFlag() const override {
		return getInterruptionFlagFor(taskOf<Interface>);
	}
	void setInterruptionFlag(bool interrupt) override {
		return setInterruptionFlagFor(taskOf<Interface>, interrupt);
	}
};

template<typename Interface>
class AbstractInterruptible : public IInterruptible<Interface>
{
public:
	virtual ~AbstractInterruptible() = default;

	bool getInterruptionFlagFor(TaskOf<Interface>) const override {
		assert(m_interruptible);
		return m_interruptible->getInterruptionFlag();
	}
	void setInterruptionFlagFor(TaskOf<Interface>, bool interrupt) override {
		assert(m_interruptible);
		m_interruptible->setInterruptionFlag(interrupt);
	}

protected:
	AbstractInterruptible(std::unique_ptr<VSIInterruptible> interruptible)
		: m_interruptible((assert(interruptible), std::move(interruptible)))
	{}
	DECL_DEF_CP_MV_CTORS_ASSIGN_BY_DEF(AbstractInterruptible)

	std::unique_ptr<VSIInterruptible> m_interruptible;
};

template<typename Class>
inline constexpr bool isThumbnailGenerator =
	std::is_base_of_v<
		tc::file_as_img::fs::IThumbnailGenerator,
		Class
	> ||
	std::is_base_of_v<
		tc::file_as_img::mem::IThumbnailGenerator,
		Class
	>;

template<typename Class>
inline constexpr bool isExporter =
	std::is_base_of_v<
		tc::file_as_img::fs::IExporter,
		Class
	> ||
	std::is_base_of_v<
		tc::file_as_img::mem::IExporter,
		Class
	>;

} //namespace tc::file_as_img
