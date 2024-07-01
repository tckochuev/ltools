set(TARGET_NAME ${PROJECT_NAME})

# Добавьте исполняемый файл
add_executable(${TARGET_NAME} main.cpp)

# Укажите включаемые каталоги
target_include_directories(${TARGET_NAME} PUBLIC 
	"${PROJECT_BINARY_DIR}" 
	"${PROJECT_SOURCE_DIR}/include"
	"C:/LEADTOOLS23/Include"
)

add_compile_definitions(${TARGET_NAME} "LTV23_CONFIG")

# Если у вас есть библиотеки в каталоге libs, раскомментируйте и обновите следующие строки
# add_subdirectory(libs)
set(LEADTOOLS_LIBDIR "C:/LEADTOOLS23/Lib/CDLL/x64")
set(LEADTOOLS_BINDIR "C:/LEADTOOLS23/Bin/CDLL/x64")
#set(LEADTOOLS_LIBS Ltfil_x.lib Ltmmx.lib Ltkrn_x.lib)
set(LEADTOOLS_LIBS Lfheif_x.lib Ltann_x.lib Ltasr_x.lib Ltaut_x.lib Ltbar_x.lib ltclr_x.lib Ltcon_x.lib Ltdic2_x.lib Ltdic_ax.lib Ltdic_x.lib Ltdis_x.lib Ltdlgclr_x.lib Ltdlgefx_x.lib Ltdlgfile_x.lib Ltdlgimgdoc_x.lib Ltdlgimgefx_x.lib Ltdlgimg_x.lib Ltdlgkrn_x.lib Ltdlgweb_x.lib LtDocWrt_x.lib Ltdrw_x.lib Ltefx_x.lib Ltfil_x.lib lticr_x.lib Ltimgclr_x.lib Ltimgcor_x.lib Ltimgefx_x.lib Ltimgopt_x.lib Ltimgsfx_x.lib Ltimgutl_x.lib Ltivwm_x.lib Ltivw_x.lib Ltjp22_x.lib Ltjp2_ax.lib Ltjp2_x.lib Ltkrn_x.lib Ltlst_x.lib ltmfuuidx.lib ltml_x.lib ltmmuuidx.lib Ltmmx.lib Ltmrc_x.lib Ltntf_x.lib Ltocr_x.lib Ltpdfcomp_x.lib Ltpdf_x.lib Ltpdg_x.lib Ltpnt_x.lib ltPrinterClientInstaller_x.lib Ltprinter_x.lib Ltregex_x.lib Ltscr_x.lib Ltsgm_x.lib ltsqlite_x.lib Ltsvg_x.lib Lttlb_x.lib Lttmb_x.lib Lttwn_x.lib Ltvdlg_x.lib Ltvkrn_x.lib LtWebKitEngine_x.lib Ltwia_x.lib Ltwvc_ax.lib Ltwvc_x.lib Ltzmv_x.lib)
list(TRANSFORM LEADTOOLS_LIBS PREPEND "${LEADTOOLS_LIBDIR}/")
target_link_libraries(${TARGET_NAME} PRIVATE 
	${LEADTOOLS_LIBS}
)