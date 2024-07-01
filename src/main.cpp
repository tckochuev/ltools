#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <string>

//#include <l_bitmap.h>
#include <lterr.h>
#include <ltmm.h>
#include <ltfil.h>

#include <stringapiset.h>

const char* MY_LICENSE_FILE("C:\\LEADTOOLS23\\license\\LEADTOOLS.lic");
const char* MY_DEVELOPER_KEY("iswHXpNThJb/bVvDd9FDk5KRCMAXLmsI2t3u3sJp/TM=");

namespace tc
{

template<typename T, typename D>
auto makeUnique(T&& t, D&& d) {
	return std::unique_ptr<std::remove_pointer_t<std::remove_reference_t<T>>, std::decay_t<D>>(
		std::forward<T>(t),
		std::forward<D>(d)
	);
}

template<typename TCode, typename BaseException = std::runtime_error>
class ExceptionWithErrorCode : public BaseException
{
public:
	using Code = TCode;

	template<typename... BaseArgs>
	ExceptionWithErrorCode(Code code, BaseArgs&&... args)
	: BaseException(std::forward<BaseArgs>(args)...), m_code(code)
	{}

	const Code& code() const {
		return m_code;
	}

protected:
	using ExceptionWithErrorCode_ = ExceptionWithErrorCode;
	Code m_code;
};

template<typename R, typename IndirectPointer>
auto makeDirectDeleter(R(*deleter)(IndirectPointer)) {
	return [deleter](std::remove_pointer_t<IndirectPointer> data) {
		deleter(&data);
	};
}

std::unique_ptr<char[]> strdup(const char* s) {
	assert(s);
	auto size = std::strlen(s);
	auto dupped = std::make_unique<char[]>(size + 1);
	std::copy(s, s + size, dupped.get());
	dupped[size] = 0;
	return dupped;
}

} //namespace tc

namespace tc::leadtools
{

class LeadToolsException : public tc::ExceptionWithErrorCode<L_INT>
{
public:
	LeadToolsException(Code code) :
		ExceptionWithErrorCode_(code, "Leadtools error: code: " + std::to_string(code) + " msg: " + makeErrorString(code))
	{}

private:
	static std::string makeErrorString(Code code) {
		auto uniqWErrorString = tc::makeUnique(ltmmGetErrorText(code), SysFreeString);
		if(std::wcslen(uniqWErrorString.get()) == 0)
		{
			return "";
		}
		else
		{
			auto multiByteSize = WideCharToMultiByte(CP_UTF8, 0, uniqWErrorString.get(), -1, nullptr, 0, nullptr, nullptr);
			assert(multiByteSize > 0);
			auto uniqErrorString = std::make_unique<char[]>(multiByteSize);
			WideCharToMultiByte(CP_UTF8, 0, uniqWErrorString.get(), -1, uniqErrorString.get(), multiByteSize, nullptr, nullptr);
			assert(uniqErrorString[multiByteSize - 1] == 0);
			return uniqErrorString.get();
		}
	}
};

template<typename F, typename... Args>
auto call(F f, Args&&... args) {
	if(auto res = std::invoke(f, std::forward<Args>(args)...); res == SUCCESS) {
		return res;
	}
	else {
		throw LeadToolsException(res);
	}
}

}

int main(int argc, char** argv)
{
	using namespace std::filesystem;
	using namespace tc::leadtools;
	try
	{
		if(argc != 3) {
			throw std::logic_error("Invalid arguments");
		}
		call(L_SetLicenseFile, tc::strdup(MY_LICENSE_FILE).get(), tc::strdup(MY_DEVELOPER_KEY).get());
		const path inputFile = argv[1];
		const path outputFile = argv[2];
		FILEINFO fileInfo{};
		//BITMAPHANDLE page1 = { 0 };
		//std::cout << "DOCUMENT: " << L_IsSupportLocked(L_SUPPORT_DOCUMENT) << std::endl;
		//call(L_LoadBitmap, inputFile.string().data(), &page1, sizeof(BITMAPHANDLE), 24, ORDER_BGR, nullptr, nullptr);
		//LOADFILEOPTION loadFileOption;
		//LoadFileOption.PageNumber = 32000;   //get number of pages in the file
        //L_FileInfo(pData->szFileName,&FileInfoSrc,sizeof(FILEINFO),FILEINFO_TOTALPAGES,&LoadFileOption);
        call(L_FileInfo, tc::strdup(inputFile.string().c_str()).get(), &fileInfo, sizeof(FILEINFO), FILEINFO_TOTALPAGES, nullptr);
        BITMAPHANDLE bitmap{};
        LOADFILEOPTION loadOpt{};
        call(L_GetDefaultLoadFileOption, &loadOpt, sizeof(LOADFILEOPTION));
        loadOpt.PageNumber = 0;
        call(L_LoadBitmap, tc::strdup(inputFile.string().c_str()).get(), &bitmap, sizeof(BITMAPHANDLE), 0, 0, &loadOpt, &fileInfo);
        call(L_SaveBitmap, tc::strdup(outputFile.string().c_str()).get(), &bitmap, FILE_PNG, 0, 0, nullptr);
        //L_SaveBitmap(tc::strdup(outputFile.string().c_str()).get(), &bitmap,
		//call(L_FileInfo, inputFile.string().data(), &fileInfo, sizeof(FILEINFO), FILEINFO_TOTALPAGES, nullptr);
		//throw LeadToolsException(ERROR_FILE_FORMAT);
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}