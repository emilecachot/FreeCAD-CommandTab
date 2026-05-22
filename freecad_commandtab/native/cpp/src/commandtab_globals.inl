
QByteArray g_lastError;
QHash<QString, QIcon> g_iconSourceCache;
QSet<QString> g_iconSourceMissCache;
QHash<QString, QIcon> g_commandIconCache;
QStyle* g_nativeScrollBarStyle = nullptr;
bool g_nativeScrollBarStyleResolved = false;

#if defined(FREECAD_COMMANDTAB_NATIVE_DEBUG)
void commandtabDebugLog(const QString& category, const QString& message)
{
    qWarning().noquote() << QStringLiteral("[FreeCADCommandTabNative][%1] %2").arg(category, message);
}
#else
void commandtabDebugLog(const QString&, const QString&)
{
}
#endif

void setLastError(const QString& error)
{
    g_lastError = error.toUtf8();
}

void clearLoadedIconCache()
{
    g_iconSourceCache.clear();
    g_iconSourceMissCache.clear();
    g_commandIconCache.clear();
}

QStyle* nativeScrollBarStyle()
{
    if (g_nativeScrollBarStyleResolved) {
        return g_nativeScrollBarStyle;
    }

    g_nativeScrollBarStyleResolved = true;

    QStringList preferredKeys;
#if defined(Q_OS_WIN)
    preferredKeys = {QStringLiteral("windows11"), QStringLiteral("windowsvista"), QStringLiteral("windows")};
#elif defined(Q_OS_MACOS)
    preferredKeys = {QStringLiteral("macos")};
#elif defined(Q_OS_LINUX)
    preferredKeys = {QStringLiteral("gtk3"), QStringLiteral("gtk2"), QStringLiteral("adwaita")};
#endif

    const QStringList availableKeys = QStyleFactory::keys();
    for (const auto& preferredKey : preferredKeys) {
        for (const auto& availableKey : availableKeys) {
            if (availableKey.compare(preferredKey, Qt::CaseInsensitive) != 0) {
                continue;
            }
            g_nativeScrollBarStyle = QStyleFactory::create(availableKey);
            if (g_nativeScrollBarStyle != nullptr) {
                if (qApp != nullptr) {
                    g_nativeScrollBarStyle->setParent(qApp);
                }
                return g_nativeScrollBarStyle;
            }
        }
    }

    return nullptr;
}
