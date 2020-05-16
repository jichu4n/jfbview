# General CPack settings.
# -----------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_VENDOR "Chuan Ji")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(
  CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "PDF and image viewer for the Linux framebuffer."
)

# Debian CPack settings.
# ----------------------
set(CPACK_DEBIAN_PACKAGE_SECTION "text")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Chuan Ji <chuan@jichu4n.com>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/jichu4n/jfbview")
set(
  CPACK_DEBIAN_PACKAGE_DEPENDS
  "libncurses5, libimlib2"
)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# Enable CPack.
# -------------
include(CPack)

