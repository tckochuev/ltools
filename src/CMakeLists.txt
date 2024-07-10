include_directories(${CMAKE_SOURCE_DIR}/include)

set(SRC
	main.cpp
	VSUtils.h
	VSError.h
	VSError.ipp
	VSError.cpp
	VSExportFileAsImages.h
	VSIInterruptible.h
	VSLeadtoolsManager.h
	VSLeadtoolsManager.cpp
	LEADTOOLS.lic.h
	LEADTOOLS.lic.key.h
)

if(APPLE)
	list(APPEND SRC VSLeadtoolsManager.mm)
endif()

add_executable(
	ltools
	${SRC}
)

#Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS system filesystem)
if(Boost_VERSION VERSION_LESS_EQUAL "1.62")
	target_compile_definitions(ltools PRIVATE _HAS_AUTO_PTR_ETC=1)
endif()
target_include_directories(ltools PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(ltools PRIVATE ${Boost_LIBRARIES})

if(APPLE)
	set(LEADTOOLS_LIBDIR
		"/Users/dev/Downloads/leadtools23_apple/Bin/Xcode/Frameworks/macOS"
	)
elseif(WIN32)
	set(LEADTOOLS_LIBDIR "C:/LEADTOOLS23/Lib/CDLL/x64")
elseif(UNIX)
	set(LEADTOOLS_LIBDIR "/home/wolfox/Documents/ltools-conan/x64")
endif()

if(NOT APPLE)
	target_compile_definitions(
		ltools PRIVATE
		"LTV23_CONFIG"
	)
endif()

if(APPLE)
	set(LEADTOOLS_LIB
		"Leadtools.Codecs.framework/Leadtools.Codecs"
		"Leadtools.framework/Leadtools"
	)
elseif(WIN32)
	set(LEADTOOLS_LIB Ltfil_x.lib Ltkrn_x.lib)
elseif(UNIX)
	set(LEADTOOLS_LIB libltfil.so libltkrn.so)
endif()

list(TRANSFORM LEADTOOLS_LIB PREPEND "${LEADTOOLS_LIBDIR}/")

if(APPLE)
	set(LEADTOOLS_INCDIR
		"${LEADTOOLS_LIBDIR}/Leadtools.Codecs.framework"
		"${LEADTOOLS_LIBDIR}/Leadtools.framework"
	)
elseif(WIN32)
    set(LEADTOOLS_INCDIR "C:/LEADTOOLS23/Include")
elseif(UNIX)
	target_compile_definitions(ltools PRIVATE FOR_LINUX)
	set(LEADTOOLS_INCDIR "/home/wolfox/Documents/ltools-conan/Include")
endif()

target_link_libraries(ltools PRIVATE "${LEADTOOLS_LIB}")
target_include_directories(ltools PRIVATE "${LEADTOOLS_INCDIR}")
