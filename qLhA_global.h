#ifndef QLHA_GLOBAL_H
#define QLHA_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QLHA_LIBRARY)
#  define QLHASHARED_EXPORT Q_DECL_EXPORT
#else
#  define QLHASHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QLHA_GLOBAL_H
