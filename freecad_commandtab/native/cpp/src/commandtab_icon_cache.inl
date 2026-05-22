QIcon loadIconFromSource(const QString& iconSource)
{
    const QString normalized = iconSource.trimmed();
    if (normalized.isEmpty()) {
        return QIcon();
    }

    const auto cachedIt = g_iconSourceCache.constFind(normalized);
    if (cachedIt != g_iconSourceCache.constEnd()) {
        return cachedIt.value();
    }
    if (g_iconSourceMissCache.contains(normalized)) {
        return QIcon();
    }

    auto renderSvgPixmap = [](const QString& source, const QSize& requestedSize = QSize(64, 64)) -> QPixmap {
        QSvgRenderer renderer(source);
        if (!renderer.isValid()) {
            QFile svgFile(source);
            if (svgFile.open(QFile::ReadOnly | QFile::Text)) {
                renderer.load(svgFile.readAll());
            }
        }
        if (!renderer.isValid()) {
            return QPixmap();
        }

        QSize renderSize = requestedSize;
        if (!renderSize.isValid() || renderSize.isEmpty()) {
            renderSize = renderer.defaultSize();
        }
        if (!renderSize.isValid() || renderSize.isEmpty()) {
            renderSize = QSize(64, 64);
        }

        const qreal dpr = (qApp != nullptr) ? qApp->devicePixelRatio() : 1.0;
        const QSize physicalSize(
            static_cast<int>(qCeil(renderSize.width() * dpr)),
            static_cast<int>(qCeil(renderSize.height() * dpr))
        );
        QPixmap pixmap(physicalSize);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter, QRectF(0, 0, renderSize.width(), renderSize.height()));
        return pixmap;
    };

    auto loadPixmapLikeFreeCAD = [&](const QString& filename, QPixmap* pixmap) -> bool {
        if (pixmap == nullptr) {
            return false;
        }

        if (QFileInfo(filename).suffix().compare(QStringLiteral("svg"), Qt::CaseInsensitive) == 0) {
            *pixmap = renderSvgPixmap(filename);
        } else {
            pixmap->load(filename);
        }

        return !pixmap->isNull();
    };

    auto loadIconFromFreeCADSearchPaths = [&](const QString& iconName) -> QIcon {
        const QString trimmed = iconName.trimmed();
        if (trimmed.isEmpty()) {
            return QIcon();
        }

        QPixmap pixmap;
        if (!loadPixmapLikeFreeCAD(trimmed, &pixmap)) {
            QList<QByteArray> formats = QImageReader::supportedImageFormats();
            formats.prepend("SVG");

            QString fileName = trimmed;
            if (!fileName.startsWith(QStringLiteral("icons:"))) {
                fileName.prepend(QStringLiteral("icons:"));
            }
            if (!loadPixmapLikeFreeCAD(fileName, &pixmap)) {
                for (QList<QByteArray>::iterator formatIt = formats.begin(); formatIt != formats.end(); ++formatIt) {
                    const QString candidatePath = QStringLiteral("%1.%2").arg(
                        fileName,
                        QString::fromLatin1((*formatIt).toLower().constData())
                    );
                    if (loadPixmapLikeFreeCAD(candidatePath, &pixmap)) {
                        break;
                    }
                }
            }
        }

        if (!pixmap.isNull()) {
            QIcon icon;
            icon.addPixmap(pixmap);
            return icon;
        }

        return QIcon();
    };

    QIcon icon;
    if (normalized.startsWith(QStringLiteral(":/"))) {
        icon = QIcon(normalized);
    } else if (normalized.startsWith(QStringLiteral("icons:"))) {
        icon = QIcon(normalized);
        if (icon.isNull()) {
            icon = loadIconFromFreeCADSearchPaths(normalized);
        }
    } else {
        const QString themeBaseName = QFileInfo(normalized).completeBaseName();
        const QString lowered = normalized.toLower();
        const QString loweredBaseName = themeBaseName.toLower();
        const QFileInfo sourceInfo(normalized);
        const bool looksLikePath =
            normalized.contains(QLatin1Char('/'))
            || normalized.contains(QLatin1Char('\\'))
            || sourceInfo.isAbsolute()
            || sourceInfo.exists();
        if (looksLikePath) {
            icon = loadIconFromFreeCADSearchPaths(normalized);
        }
        if (icon.isNull()) {
            icon = loadIconFromFreeCADSearchPaths(normalized);
        }
        if (icon.isNull() && lowered != normalized) {
            icon = loadIconFromFreeCADSearchPaths(lowered);
        }
        if (icon.isNull() && QIcon::hasThemeIcon(normalized)) {
            icon = QIcon::fromTheme(normalized);
        }
        if (icon.isNull() && lowered != normalized && QIcon::hasThemeIcon(lowered)) {
            icon = QIcon::fromTheme(lowered);
        }
        if (
            icon.isNull()
            && !themeBaseName.isEmpty()
            && themeBaseName != normalized
            && QIcon::hasThemeIcon(themeBaseName)
        ) {
            icon = QIcon::fromTheme(themeBaseName);
        }
        if (
            icon.isNull()
            && !loweredBaseName.isEmpty()
            && loweredBaseName != lowered
            && QIcon::hasThemeIcon(loweredBaseName)
        ) {
            icon = QIcon::fromTheme(loweredBaseName);
        }
    }

    if (!icon.isNull()) {
        g_iconSourceCache.insert(normalized, icon);
        g_iconSourceMissCache.remove(normalized);
    } else {
        g_iconSourceMissCache.insert(normalized);
    }
    return icon;
}

struct CommandTabCommandEntry
{
    QString type;
    QString id;
    QString text;
    QString iconPath;
    QString shortcut;
    QString size;
    bool textVisible = true;
    bool textVisibilityExplicit = false;
    QString sourceWorkbenchId;
    QString sourceToolbarTitle;
    QVector<CommandTabCommandEntry> menuCommands;
};

QStringList commandIconCandidates(const QString& commandId);
QStringList workbenchIconCandidates(const QString& workbenchId, const QString& workbenchTitle);
QString resolveWorkbenchIconSource(
    const QString& addonRootPath,
    const QString& workbenchId,
    const QString& workbenchTitle
);

QString staleIconStemCandidate(const QString& iconPath)
{
    const QFileInfo storedInfo(iconPath.trimmed());
    if (!storedInfo.isAbsolute() || storedInfo.exists()) {
        return QString();
    }

    const QString stem = storedInfo.completeBaseName();
    if (stem.isEmpty()) {
        return QString();
    }
    const qsizetype lastUnderscore = stem.lastIndexOf(QLatin1Char('_'));
    if (lastUnderscore > 0 && stem.size() - lastUnderscore - 1 == 12) {
        return stem.left(lastUnderscore);
    }
    return stem;
}

QIcon genericCommandFallbackIcon()
{
    static QIcon cachedFallbackIcon;
    if (!cachedFallbackIcon.isNull()) {
        return cachedFallbackIcon;
    }

    if (qApp != nullptr && qApp->style() != nullptr) {
        const QIcon styleIcon = qApp->style()->standardIcon(QStyle::SP_FileIcon);
        if (!styleIcon.isNull()) {
            cachedFallbackIcon = styleIcon;
            return cachedFallbackIcon;
        }
    }

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(QStringLiteral("#7c8a9a")), 1.4));
    painter.setBrush(QColor(QStringLiteral("#2b3542")));
    painter.drawRoundedRect(QRectF(3.0, 3.0, 26.0, 26.0), 5.0, 5.0);
    painter.setPen(QColor(QStringLiteral("#dfe6ef")));
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(12);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, QStringLiteral("?"));
    cachedFallbackIcon.addPixmap(pixmap);
    return cachedFallbackIcon;
}

QString commandIconCacheKey(const CommandTabCommandEntry& command)
{
    QStringList menuIds;
    menuIds.reserve(command.menuCommands.size());
    for (const auto& menuCommand : command.menuCommands) {
        const QString nestedId = menuCommand.id.trimmed();
        if (!nestedId.isEmpty()) {
            menuIds.push_back(nestedId);
        }
    }
    return QStringLiteral("%1|%2|%3|%4").arg(
        command.type.trimmed(),
        command.id.trimmed(),
        command.iconPath.trimmed(),
        menuIds.join(QStringLiteral(","))
    );
}

QIcon loadCommandEntryIcon(const CommandTabCommandEntry& command)
{
    const QString cacheKey = commandIconCacheKey(command);
    const auto commandCacheIt = g_commandIconCache.constFind(cacheKey);
    if (commandCacheIt != g_commandIconCache.constEnd()) {
        return commandCacheIt.value();
    }

    const QString explicitIconPath = command.iconPath.trimmed();
    if (!explicitIconPath.isEmpty()) {
        const QIcon directIcon = loadIconFromSource(explicitIconPath);
        if (!directIcon.isNull()) {
            g_commandIconCache.insert(cacheKey, directIcon);
            return directIcon;
        }

        const QString staleStemCandidate = staleIconStemCandidate(explicitIconPath);
        if (!staleStemCandidate.isEmpty()) {
            const QIcon staleStemIcon = loadIconFromSource(staleStemCandidate);
            if (!staleStemIcon.isNull()) {
                commandtabDebugLog(
                    QStringLiteral("icon-fallback"),
                    QStringLiteral("Recovered stale icon path for '%1' using stem '%2'")
                        .arg(command.id, staleStemCandidate)
                );
                g_commandIconCache.insert(cacheKey, staleStemIcon);
                return staleStemIcon;
            }
        }
    }

    for (const auto& candidate : commandIconCandidates(command.id)) {
        const QIcon canonicalIcon = loadIconFromSource(candidate);
        if (!canonicalIcon.isNull()) {
            g_commandIconCache.insert(cacheKey, canonicalIcon);
            return canonicalIcon;
        }
    }

    if (command.type == QStringLiteral("dropdown")) {
        for (const auto& menuCommand : command.menuCommands) {
            const QIcon nestedIcon = loadCommandEntryIcon(menuCommand);
            if (!nestedIcon.isNull()) {
                commandtabDebugLog(
                    QStringLiteral("icon-fallback"),
                    QStringLiteral("Dropdown '%1' uses nested command icon '%2'")
                        .arg(command.id, menuCommand.id)
                );
                g_commandIconCache.insert(cacheKey, nestedIcon);
                return nestedIcon;
            }
        }
    }

    commandtabDebugLog(
        QStringLiteral("icon-fallback"),
        QStringLiteral("Using generic fallback icon for '%1'").arg(command.id)
    );
    const QIcon fallbackIcon = genericCommandFallbackIcon();
    if (!fallbackIcon.isNull()) {
        g_commandIconCache.insert(cacheKey, fallbackIcon);
    }
    return fallbackIcon;
}
