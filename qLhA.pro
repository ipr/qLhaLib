#-------------------------------------------------
#
# Project created by QtCreator 2011-04-29T23:31:32
#
#-------------------------------------------------

QT       -= gui

TARGET = qLhA
TEMPLATE = lib

DEFINES += QLHA_LIBRARY 
DEFINES += _CRT_SECURE_NO_WARNINGS

SOURCES += qlhalib.cpp \
    AnsiFile.cpp \
    LhArchive.cpp \
    crcio.cpp \
    LhHeader.cpp

# src/util.c 
# src/lhlist.c 
# src/lhext.c 
# src/lhadd.c 
# src/larc.c 
# src/extract.c 
# src/append.c 
# src/lharc_main.c
# bitio.cpp 
# lhdir.cpp 
# LzEncode.cpp 
# Huffman.cpp
# LzDecode.cpp 


HEADERS += qlhalib.h\
        qLhA_global.h \
    AnsiFile.h \
    LhArchive.h \
    GenericTime.h \
    LhaTypeDefs.h \
    crcio.h \
    FiletimeHelper.h \
    LhHeader.h

# src/lha_macro.h 
# src/lha_main.h 
# bitio.h 
# lhdir.h 
# LzEncode.h 
# Huffman.h
# LzDecode.h 
