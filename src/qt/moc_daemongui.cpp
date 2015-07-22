/****************************************************************************
** Meta object code from reading C++ file 'daemongui.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "daemongui.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'daemongui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DBBDaemonGui_t {
    QByteArrayData data[9];
    char stringdata[101];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DBBDaemonGui_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DBBDaemonGui_t qt_meta_stringdata_DBBDaemonGui = {
    {
QT_MOC_LITERAL(0, 0, 12), // "DBBDaemonGui"
QT_MOC_LITERAL(1, 13, 17), // "showCommandResult"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 6), // "result"
QT_MOC_LITERAL(4, 39, 12), // "eraseClicked"
QT_MOC_LITERAL(5, 52, 10), // "ledClicked"
QT_MOC_LITERAL(6, 63, 13), // "setResultText"
QT_MOC_LITERAL(7, 77, 18), // "setPasswordClicked"
QT_MOC_LITERAL(8, 96, 4) // "seed"

    },
    "DBBDaemonGui\0showCommandResult\0\0result\0"
    "eraseClicked\0ledClicked\0setResultText\0"
    "setPasswordClicked\0seed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DBBDaemonGui[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   47,    2, 0x0a /* Public */,
       5,    0,   48,    2, 0x0a /* Public */,
       6,    1,   49,    2, 0x0a /* Public */,
       7,    0,   52,    2, 0x0a /* Public */,
       8,    0,   53,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DBBDaemonGui::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DBBDaemonGui *_t = static_cast<DBBDaemonGui *>(_o);
        switch (_id) {
        case 0: _t->showCommandResult((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->eraseClicked(); break;
        case 2: _t->ledClicked(); break;
        case 3: _t->setResultText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->setPasswordClicked(); break;
        case 5: _t->seed(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (DBBDaemonGui::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&DBBDaemonGui::showCommandResult)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject DBBDaemonGui::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_DBBDaemonGui.data,
      qt_meta_data_DBBDaemonGui,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DBBDaemonGui::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DBBDaemonGui::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DBBDaemonGui.stringdata))
        return static_cast<void*>(const_cast< DBBDaemonGui*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int DBBDaemonGui::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void DBBDaemonGui::showCommandResult(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
