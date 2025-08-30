SUMMARY = "Update Library - D-Bus free RAUC alternative with bootchooser integration"
DESCRIPTION = "A simplified RAUC implementation without D-Bus dependency, featuring GRUB bootchooser integration and automatic reboot functionality"
HOMEPAGE = "https://github.com/your-project/update-library"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "glib-2.0 openssl pkgconfig-native"
RDEPENDS:${PN} = "glib-2.0 openssl grub"

PV = "1.0.0"
PR = "r1"

# 소스 파일 위치 지정 (local 디렉토리)
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../local/:"
SRC_URI = "file://update-library"

S = "${WORKDIR}/update-library"

# CMake 빌드 시스템 상속
inherit cmake

# CMake 설정
EXTRA_OECMAKE += " \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_DLT=OFF \
    -DCMAKE_CROSSCOMPILING=TRUE \
"

# pkg-config 환경 변수 설정
export PKG_CONFIG_PATH = "${STAGING_LIBDIR}/pkgconfig:${STAGING_DATADIR}/pkgconfig"

# 설치 대상 디렉토리 설정
INSTALL_DIR = "${D}${bindir}"

do_install() {
    install -d ${D}${bindir}
    install -d ${D}${libdir}
    install -d ${D}${includedir}/rauc
    
    # 실행 파일 설치
    if [ -f ${B}/update-test-app ]; then
        install -m 0755 ${B}/update-test-app ${D}${bindir}/update-library
    fi
    
    # 라이브러리 설치
    if [ -f ${B}/libupdate-library.a ]; then
        install -m 0644 ${B}/libupdate-library.a ${D}${libdir}/
    fi
    
    # 헤더 파일 설치
    for header in ${S}/include/rauc/*.h; do
        if [ -f "$header" ]; then
            install -m 0644 "$header" ${D}${includedir}/rauc/
        fi
    done
}

# 패키지 분할 설정
PACKAGES = "${PN} ${PN}-dev ${PN}-staticdev ${PN}-dbg"

FILES:${PN} = "${bindir}/update-library"
FILES:${PN}-dev = "${includedir}"
FILES:${PN}-staticdev = "${libdir}/*.a"
FILES:${PN}-dbg = "${bindir}/.debug/*"

# 개발 패키지에 대한 의존성
RDEPENDS:${PN}-dev = "${PN} (= ${EXTENDPKGV})"
RDEPENDS:${PN}-staticdev = "${PN}-dev (= ${EXTENDPKGV})"

# 보안 관련 설정
INSANE_SKIP:${PN} = "already-stripped"

# 패키지 설명
SUMMARY:${PN} = "Update Library runtime"
SUMMARY:${PN}-dev = "Update Library development files"
SUMMARY:${PN}-staticdev = "Update Library static libraries"