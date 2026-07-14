if(CONFIG_ZMK_DISPLAY)
  zephyr_library()
  zephyr_library_sources(src/dp65_display_boot.c)
endif()
