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
    LhHeader.cpp \
    LhExtract.cpp \
    LhDecoder.cpp \
    Huffman.cpp

# LzEncode.cpp 
# Huffman.cpp


HEADERS += qlhalib.h\
        qLhA_global.h \
    AnsiFile.h \
    LhArchive.h \
    GenericTime.h \
    LhaTypeDefs.h \
    crcio.h \
    FiletimeHelper.h \
    LhHeader.h \
    LhExtract.h \
    LhDecoder.h \
    Huffman.h \
    FilemodeFlags.h

# LzEncode.h 
# Huffman.h
