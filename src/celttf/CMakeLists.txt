set(CELTTF_SOURCES
  fontmanager.cpp
  fontmanager.h
  truetypefont.cpp
  truetypefont.h
)

add_library(celttf STATIC ${CELTTF_SOURCES})
