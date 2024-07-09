include_directories(${CMAKE_SOURCE_DIR}/include)

#Boost
find_package(Boost REQUIRED COMPONENTS regex program_options)
if(Boost_VERSION VERSION_LESS_EQUAL "1.62")
	target_compile_definitions(ltools PRIVATE _HAS_AUTO_PTR_ETC=1)
endif()

add_executable(
	ltools
	main.cpp
	VSUtils.h
	VSExportFileAsImages.h
	VSIInterruptible.h
	VSLeadtoolsManager.h
	VSLeadtoolsManager.cpp
	VSLeadtoolsManager.mm
	LEADTOOLS.lic.h
	LEADTOOLS.lic.key.h
)

target_include_directories(ltools PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(ltools PRIVATE ${Boost_LIBRARIES})

set(LEADTOOLS_LIBDIR
	"/Users/dev/Downloads/leadtools23_apple/Bin/Xcode/Frameworks/macOS"
)

set(LEADTOOLS_LIB
	"${LEADTOOLS_LIBDIR}/Leadtools.Codecs.framework/Leadtools.Codecs"
	"${LEADTOOLS_LIBDIR}/Leadtools.framework/Leadtools"
	#${LEADTOOLS_LIBDIR}/Leadtools.Codecs.framework
)

set(LEADTOOLS_INCDIR
	#"${LEADTOOLS_LIBDIR}/Leadtools.Codecs.framework/Headers"
	"${LEADTOOLS_LIBDIR}/Leadtools.Codecs.framework"
	"${LEADTOOLS_LIBDIR}/Leadtools.framework"
)

target_link_libraries(ltools PRIVATE "${LEADTOOLS_LIB}")
target_include_directories(ltools PRIVATE "${LEADTOOLS_INCDIR}")
