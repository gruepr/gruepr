// xlsxglobal.h

#ifndef XLSXGLOBAL_H
#define XLSXGLOBAL_H

// PATCHED for gruepr (2026-08-09): upstream QXlsx defines `stdext` as a macro for `::std` here, as a
// compatibility shim for very old MSVC/Dinkumware STL versions that predate this codebase's toolset.
// Nothing in QXlsx actually calls stdext::make_checked_array_iterator (confirmed by search) -- the
// macro is unused dead code -- but on modern MSVC (tested: VS2022/14.35) it collides with the
// standard library's own internal use of the `stdext` identifier inside <iterator>, corrupting its
// parse with a cascade of syntax errors. Removed rather than guarded, since it serves no purpose here.

#include <cstdio>
#include <iostream>
#include <string>

#include <QByteArray>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

#if defined(QXlsx_SHAREDLIB)
#    if defined(QXlsx_EXPORTS)
#        define QXLSX_EXPORT Q_DECL_EXPORT
#    else
#        define QXLSX_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define QXLSX_EXPORT
#endif

#define QT_BEGIN_NAMESPACE_XLSX namespace QXlsx {
#define QT_END_NAMESPACE_XLSX }

#define QXLSX_USE_NAMESPACE using namespace QXlsx;

#endif // XLSXGLOBAL_H
