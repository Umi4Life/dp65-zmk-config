# Build into the ZMK app target so zmk/display.h and drivers/ext_power.h resolve.
if(CONFIG_ZMK_DISPLAY)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/dp65_display_boot.c)
endif()
