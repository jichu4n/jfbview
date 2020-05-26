# Manpage.
# --------
add_custom_target(
  jfbview.1.gz
  ALL
  COMMAND
    cat "${CMAKE_CURRENT_SOURCE_DIR}/jfbview.1" |
    gzip > "${CMAKE_CURRENT_BINARY_DIR}/jfbview.1.gz"
  DEPENDS jfbview.1
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/jfbview.1.gz"
  DESTINATION share/man/man1
)

# README.
# -------
install(FILES README.md DESTINATION share/doc/jfbview)

