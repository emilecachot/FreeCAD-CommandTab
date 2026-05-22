#include <QtGlobal>

#ifdef Q_OS_MACOS

#include <dlfcn.h>

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QObject>
#include <QVariant>

namespace {

template <typename Function>
Function resolveQtSymbol(const char* symbolName)
{
    return reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, symbolName));
}

}  // namespace

extern "C" void freecad_commandtab_qt68_painter_do_set_brush(
    QPainter* painter,
    const QBrush& brush,
    QBrush*
) asm("__ZN8QPainter10doSetBrushERK6QBrushPS0_");

extern "C" void freecad_commandtab_qt68_painter_do_set_brush(
    QPainter* painter,
    const QBrush& brush,
    QBrush*
)
{
    using SetBrushFn = void (*)(QPainter*, const QBrush&);
    static SetBrushFn setBrush = resolveQtSymbol<SetBrushFn>("__ZN8QPainter8setBrushERK6QBrush");
    if (painter != nullptr && setBrush != nullptr) {
        setBrush(painter, brush);
    }
}

extern "C" void freecad_commandtab_qt68_painter_do_set_pen(
    QPainter* painter,
    const QPen& pen,
    QPen*
) asm("__ZN8QPainter8doSetPenERK4QPenPS0_");

extern "C" void freecad_commandtab_qt68_painter_do_set_pen(
    QPainter* painter,
    const QPen& pen,
    QPen*
)
{
    using SetPenFn = void (*)(QPainter*, const QPen&);
    static SetPenFn setPen = resolveQtSymbol<SetPenFn>("__ZN8QPainter6setPenERK4QPen");
    if (painter != nullptr && setPen != nullptr) {
        setPen(painter, pen);
    }
}

extern "C" void freecad_commandtab_qt68_painter_set_brush_color(
    QPainter* painter,
    QColor color
) asm("__ZN8QPainter8setBrushE6QColor");

extern "C" void freecad_commandtab_qt68_painter_set_brush_color(
    QPainter* painter,
    QColor color
)
{
    freecad_commandtab_qt68_painter_do_set_brush(painter, QBrush(color), nullptr);
}

extern "C" bool freecad_commandtab_qt68_object_do_set_property(
    QObject* object,
    const char* name,
    const QVariant& value,
    QVariant*
) asm("__ZN7QObject13doSetPropertyEPKcRK8QVariantPS2_");

extern "C" bool freecad_commandtab_qt68_object_do_set_property(
    QObject* object,
    const char* name,
    const QVariant& value,
    QVariant*
)
{
    using SetPropertyFn = bool (*)(QObject*, const char*, const QVariant&);
    static SetPropertyFn setProperty = resolveQtSymbol<SetPropertyFn>(
        "__ZN7QObject11setPropertyEPKcRK8QVariant"
    );
    if (object == nullptr || setProperty == nullptr) {
        return false;
    }
    return setProperty(object, name, value);
}

#endif
