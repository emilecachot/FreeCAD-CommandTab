class CommandTabCommandButton final : public QWidget
{
public:
    explicit CommandTabCommandButton(
        const CommandTabCommandEntry& command,
        const CommandTabModel::CommandTabTheme* theme,
        QWidget* parent = nullptr
    )
        : QWidget(parent)
        , m_theme(theme)
        , m_rawText(command.text.simplified())
        , m_menuCommands(command.menuCommands)
        , m_buttonSize(parseCommandTabButtonSizeKind(command.size))
        , m_textVisible(command.textVisible)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setAutoFillBackground(false);

        m_commandId = command.id;
        m_icon = loadCommandEntryIcon(command);

        QFont nextFont = font();
        nextFont.setPointSize(std::max(7, scaledPx(10)));
        setFont(nextFont);
        m_iconOnlySize = defaultIconOnlySizeForButton();

        if (!m_textVisible) {
            m_textLines.clear();
            m_displayText.clear();
        } else if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            m_textLines = wrapCommandTabTextToWidth(
                m_rawText,
                QFontMetrics(nextFont),
                scaledPx(136),
                maxDisplayLines()
            );
            m_displayText = m_textLines.join(QStringLiteral("\n"));
        } else if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
            m_textLines = wrapCommandTabTextToWidth(
                m_rawText,
                QFontMetrics(nextFont),
                scaledPx(118),
                maxDisplayLines()
            );
            m_displayText = m_textLines.join(QStringLiteral("\n"));
        } else {
            m_textLines = wrapCommandTabTextToWidth(
                m_rawText,
                QFontMetrics(nextFont),
                scaledPx(104),
                maxDisplayLines()
            );
            m_displayText = m_textLines.join(QStringLiteral("\n"));
        }
        const QString tooltipBase = m_rawText.isEmpty() ? command.id : m_rawText;
        const QString normalizedShortcut = command.shortcut.trimmed();
        setToolTip(normalizedShortcut.isEmpty()
            ? tooltipBase
            : QStringLiteral("%1\n[%2]").arg(tooltipBase, normalizedShortcut));

        m_preferredSize = computePreferredSize();
        setMinimumSize(QSize(0, 0));
        resize(m_preferredSize);
    }

    void setTriggeredHandler(std::function<void()> handler)
    {
        m_triggeredHandler = std::move(handler);
    }

    void setMenuCommandHandler(std::function<void(const QString&)> handler)
    {
        m_menuCommandHandler = std::move(handler);
    }

    void setActionResolver(std::function<QAction*(const QString&)> resolver)
    {
        m_actionResolver = std::move(resolver);
    }

    void setContextCommandHandler(std::function<void(const QString&)> handler)
    {
        m_contextCommandHandler = std::move(handler);
    }

    void setInQuickAccess(bool inQuickAccess)
    {
        m_inQuickAccess = inQuickAccess;
    }

    QSize sizeHint() const override
    {
        return m_preferredSize;
    }

    QSize minimumSizeHint() const override
    {
        return m_preferredSize;
    }

    bool isLargeButton() const
    {
        return m_buttonSize == CommandTabButtonSizeKind::Large;
    }

    bool isMediumButton() const
    {
        return m_buttonSize == CommandTabButtonSizeKind::Medium;
    }

    bool showsText() const
    {
        return m_textVisible;
    }

    void setTextVisible(bool visible)
    {
        if (m_textVisible == visible) {
            return;
        }
        m_textVisible = visible;
        m_assignedWidth = 0;
        recomputeLayout();
    }

    void applySettings(const CommandTabSettingsState& settings)
    {
        const int nextDisplayScalePercent = std::clamp(settings.commandtabScalePercent, 60, 140);
        auto scaledSetting = [nextDisplayScalePercent](int value) -> int {
            return std::max(
                1,
                qRound(static_cast<qreal>(value) * static_cast<qreal>(nextDisplayScalePercent) / 100.0)
            );
        };
        auto iconOnlySizeForButton = [this, &settings]() -> int {
            int value = 20;
            switch (m_buttonSize) {
                case CommandTabButtonSizeKind::Large:
                    value = settings.iconOnlySizeLarge;
                    break;
                case CommandTabButtonSizeKind::Medium:
                    value = settings.iconOnlySizeMedium;
                    break;
                case CommandTabButtonSizeKind::Small:
                default:
                    value = settings.iconOnlySizeSmall;
                    break;
            }
            const int scaledValue = std::max(
                8,
                qRound(
                    static_cast<qreal>(std::clamp(value, 12, 64))
                    * static_cast<qreal>(std::clamp(settings.commandtabScalePercent, 60, 140))
                    / 100.0
                )
            );
            return std::clamp(scaledValue, 8, 96);
        };
        const int nextCompactPadding = std::clamp(
            scaledSetting(std::clamp(settings.compactButtonPadding, 1, 8)),
            1,
            12
        );

        bool nextTextVisible = m_textVisible;
        switch (m_buttonSize) {
            case CommandTabButtonSizeKind::Medium:
                nextTextVisible = settings.showIconTextMedium;
                break;
            case CommandTabButtonSizeKind::Large:
                nextTextVisible = settings.showIconTextLarge;
                break;
            case CommandTabButtonSizeKind::Small:
            default:
                nextTextVisible = settings.showIconTextSmall;
                break;
        }

        const int nextIconOnlySize = iconOnlySizeForButton();
        const int nextFontSize = std::clamp(scaledSetting(10), 7, 22);
        if (
            m_textVisible == nextTextVisible
            && m_iconOnlySize == nextIconOnlySize
            && m_compactButtonPadding == nextCompactPadding
            && m_displayScalePercent == nextDisplayScalePercent
        ) {
            return;
        }
        m_textVisible = nextTextVisible;
        m_iconOnlySize = nextIconOnlySize;
        m_compactButtonPadding = nextCompactPadding;
        m_displayScalePercent = nextDisplayScalePercent;
        QFont nextFont = font();
        nextFont.setPointSize(nextFontSize);
        setFont(nextFont);
        m_assignedWidth = 0;
        recomputeLayout();
    }

    bool prefersWideSmallLayout() const
    {
        if (!m_textVisible) {
            return false;
        }
        if (m_buttonSize != CommandTabButtonSizeKind::Small) {
            return false;
        }

        const QFontMetrics metrics(font());
        const QStringList compactLines = wrapCommandTabTextExact(m_rawText, metrics, scaledPx(118));
        if (compactLines.size() > maxDisplayLines()) {
            return true;
        }

        if (metrics.horizontalAdvance(m_rawText) > scaledPx(118)) {
            return true;
        }

        const QStringList wrappedCompact = wrapCommandTabSmallText(
            m_rawText, metrics, scaledPx(118), maxDisplayLines()
        );
        return commandtabLinesElided(wrappedCompact);
    }

    bool hasMenuCommands() const
    {
        return !m_menuCommands.isEmpty();
    }

    int idealPanelWidth() const
    {
        if (!m_textVisible) {
            return std::max(compactButtonWidth(), m_preferredSize.width());
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            return std::max(scaledPx(116), m_preferredSize.width());
        }

        if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
            const QFontMetrics metrics(font());
            for (int candidateWidth = scaledPx(102); candidateWidth <= scaledPx(186); candidateWidth += std::max(1, scaledPx(6))) {
                const int textWidth = std::max(scaledPx(56), candidateWidth - scaledPx(8) - compactIconBandWidth() - scaledPx(10) - scaledPx(8));
                const QStringList wrapped = wrapCommandTabTextExact(m_rawText, metrics, textWidth);
                if (!wrapped.isEmpty() && wrapped.size() <= maxDisplayLines()) {
                    return candidateWidth;
                }
            }
            return scaledPx(186);
        }

        const QFontMetrics metrics(font());
        for (int candidateWidth = scaledPx(84); candidateWidth <= scaledPx(156); candidateWidth += std::max(1, scaledPx(6))) {
            const int textWidth = std::max(scaledPx(50), candidateWidth - scaledPx(8) - compactIconBandWidth() - scaledPx(10) - scaledPx(8));
            const QStringList wrapped = wrapCommandTabTextExact(m_rawText, metrics, textWidth);
            if (!wrapped.isEmpty() && wrapped.size() <= maxDisplayLines()) {
                return candidateWidth;
            }
        }
        return scaledPx(156);
    }

    int idealWidePanelWidth() const
    {
        if (!m_textVisible) {
            return idealPanelWidth();
        }
        if (m_buttonSize != CommandTabButtonSizeKind::Small) {
            return idealPanelWidth();
        }

        const QFontMetrics metrics(font());
        for (int candidateWidth = scaledPx(96); candidateWidth <= scaledPx(198); candidateWidth += std::max(1, scaledPx(6))) {
            const int textWidth = std::max(scaledPx(56), candidateWidth - scaledPx(8) - compactIconBandWidth() - scaledPx(10) - scaledPx(8));
            const QStringList wrapped = wrapCommandTabTextExact(m_rawText, metrics, textWidth);
            if (!wrapped.isEmpty() && wrapped.size() <= maxDisplayLines()) {
                return candidateWidth;
            }
        }
        return scaledPx(198);
    }

    void applyPanelWidth(int width)
    {
        if (!m_textVisible) {
            const int compactWidth = compactButtonWidth();
            if (m_assignedWidth == compactWidth) {
                return;
            }
            m_assignedWidth = compactWidth;
            recomputeLayout();
            return;
        }

        const int minimumWidth = m_buttonSize == CommandTabButtonSizeKind::Large
            ? scaledPx(116)
            : (m_buttonSize == CommandTabButtonSizeKind::Medium ? scaledPx(102) : scaledPx(84));
        const int normalizedWidth = std::max(width, minimumWidth);
        if (normalizedWidth == m_assignedWidth) {
            return;
        }

        m_assignedWidth = normalizedWidth;
        recomputeLayout();
    }

    void setIconOverride(const QIcon& icon)
    {
        if (icon.isNull()) {
            return;
        }
        m_icon = icon;
        update();
    }

    void setChecked(bool checked)
    {
        if (m_checked == checked) {
            return;
        }
        m_checked = checked;
        update();
    }

    bool isChecked() const
    {
        return m_checked;
    }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override
#else
    void enterEvent(QEvent* event) override
#endif
    {
        m_hovered = true;
        update();
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        m_hovered = false;
        m_pressed = false;
        update();
        QWidget::leaveEvent(event);
    }

    void focusInEvent(QFocusEvent* event) override
    {
        update();
        QWidget::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        update();
        QWidget::focusOutEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        if (m_commandId.isEmpty() || m_contextCommandHandler == nullptr) {
            QWidget::contextMenuEvent(event);
            return;
        }
        QMenu menu(this);
        const QString toggleLabel = m_inQuickAccess
            ? QCoreApplication::translate("CommandTabCommandButton", "Remove from quick access")
            : QCoreApplication::translate("CommandTabCommandButton", "Add to quick access");
        QAction* toggleAction = menu.addAction(toggleLabel);
        connect(toggleAction, &QAction::triggered, this, [this]() {
            m_contextCommandHandler(
                QStringLiteral("__commandtab_quick_access_toggle__:")
                + encodeCommandTabCommandArgument(m_commandId)
            );
        });
        menu.exec(event->globalPos());
        event->accept();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
            if (hasMenuCommands()) {
                showMenu();
                event->accept();
                return;
            }
            if (m_triggeredHandler) {
                m_triggeredHandler();
            }
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_pressed = true;
            update();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        const bool shouldTrigger =
            m_pressed && event->button() == Qt::LeftButton && rect().contains(event->pos());
        m_pressed = false;
        update();
        if (shouldTrigger) {
            if (hasMenuCommands()) {
                showMenu();
                event->accept();
                return;
            }
            if (m_triggeredHandler) {
                m_triggeredHandler();
            }
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (m_buttonSize == CommandTabButtonSizeKind::Large && m_textVisible) {
            paintLargeButton(&painter);
            return;
        }
        paintCompactButton(&painter);
    }

private:
    int defaultIconOnlySizeForButton() const
    {
        switch (m_buttonSize) {
            case CommandTabButtonSizeKind::Large:
                return scaledPx(32);
            case CommandTabButtonSizeKind::Medium:
                return scaledPx(24);
            case CommandTabButtonSizeKind::Small:
            default:
                return scaledPx(20);
        }
    }

    int compactButtonWidth() const
    {
        if (!m_textVisible) {
            int minimumEdge = scaledPx(30);
            if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
                minimumEdge = scaledPx(32);
            } else if (m_buttonSize == CommandTabButtonSizeKind::Large) {
                minimumEdge = scaledPx(34);
            }
            return std::max(minimumEdge, m_iconOnlySize + m_compactButtonPadding * 2 + 2);
        }
        return std::max(scaledPx(52), m_iconOnlySize + (m_compactButtonPadding + 1) * 2 + 2);
    }

    int maxDisplayLines() const
    {
        return 2;
    }

    int compactIconBandWidth() const
    {
        if (!m_textVisible) {
            return std::max(m_iconOnlySize, compactButtonWidth() - 2);
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
            return std::max(scaledPx(32), std::clamp(m_iconOnlySize + scaledPx(8), scaledPx(28), scaledPx(44)));
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            return std::max(scaledPx(34), std::clamp(m_iconOnlySize + scaledPx(10), scaledPx(30), scaledPx(50)));
        }
        return std::max(scaledPx(30), std::clamp(m_iconOnlySize + scaledPx(8), scaledPx(26), scaledPx(42)));
    }

    int compactIconEdge() const
    {
        if (!m_textVisible) {
            return m_iconOnlySize;
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            return std::clamp(m_iconOnlySize + scaledPx(8), scaledPx(24), scaledPx(56));
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
            return std::clamp(m_iconOnlySize + scaledPx(4), scaledPx(20), scaledPx(40));
        }
        return std::clamp(m_iconOnlySize + scaledPx(6), scaledPx(18), scaledPx(36));
    }

    int fixedButtonHeight(const QFontMetrics& metrics) const
    {
        if (!m_textVisible) {
            Q_UNUSED(metrics);
            return compactButtonWidth();
        }
        const int reservedTextHeight = commandtabReservedTextHeight(metrics);
        if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            const int largeIconEdge = std::clamp(m_iconOnlySize + scaledPx(8), scaledPx(24), scaledPx(56));
            const int iconBlockHeight = largeIconEdge + scaledPx(8);
            return std::max(
                {
                    scaledPx(82),
                    scaledPx(48) + reservedTextHeight + scaledPx(2),
                    iconBlockHeight + reservedTextHeight + scaledPx(4),
                }
            );
        }
        return std::max(scaledPx(44), reservedTextHeight + scaledPx(4));
    }

    void recomputeLayout()
    {
        const QFontMetrics metrics(font());
        if (!m_textVisible) {
            m_textLines.clear();
        } else if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            const int textWidth = std::max(scaledPx(64), m_assignedWidth - scaledPx(18));
            m_textLines = wrapCommandTabTextToWidth(m_rawText, metrics, textWidth, maxDisplayLines());
        } else if (m_buttonSize == CommandTabButtonSizeKind::Medium) {
            const int textWidth = std::max(
                scaledPx(56),
                m_assignedWidth - scaledPx(8) - compactIconBandWidth() - scaledPx(10) - scaledPx(8)
            );
            const QStringList exactLines = wrapCommandTabTextExact(m_rawText, metrics, textWidth);
            if (exactLines.size() <= maxDisplayLines()) {
                m_textLines = exactLines;
            } else {
                m_textLines = wrapCommandTabSmallText(m_rawText, metrics, textWidth, maxDisplayLines());
            }
        } else {
            const int textWidth = std::max(
                scaledPx(50),
                m_assignedWidth - scaledPx(8) - compactIconBandWidth() - scaledPx(10) - scaledPx(8)
            );
            const QStringList exactLines = wrapCommandTabTextExact(m_rawText, metrics, textWidth);
            if (exactLines.size() <= maxDisplayLines()) {
                m_textLines = exactLines;
            } else {
                m_textLines = wrapCommandTabSmallText(m_rawText, metrics, textWidth, maxDisplayLines());
            }
        }

        if (m_textVisible && m_textLines.isEmpty() && !m_rawText.isEmpty()) {
            m_textLines.push_back(m_rawText);
        }

        m_displayText = m_textLines.join(QStringLiteral("\n"));
        m_preferredSize = computePreferredSize();
        setMinimumSize(QSize(0, 0));
        resize(m_preferredSize);
        updateGeometry();
        update();
    }

    QSize computePreferredSize() const
    {
        const QFontMetrics metrics(font());
        const int buttonHeight = fixedButtonHeight(metrics);
        if (!m_textVisible) {
            const int compactWidth = compactButtonWidth();
            const int edge = std::max(compactWidth, buttonHeight);
            return QSize(edge, edge);
        }
        if (m_buttonSize == CommandTabButtonSizeKind::Large) {
            const int textWidth = std::max(
                scaledPx(64),
                (m_assignedWidth > 0 ? m_assignedWidth : scaledPx(110)) - scaledPx(18)
            );
            const QRect textBounds = metrics.boundingRect(
                QRect(0, 0, textWidth, 1000),
                Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                m_displayText
            );
            const int width = std::max(
                m_assignedWidth > 0 ? m_assignedWidth : scaledPx(110),
                textBounds.width() + scaledPx(20)
            );
            return QSize(width, buttonHeight);
        }

        if (m_displayText.isEmpty()) {
            return QSize(scaledPx(44), buttonHeight);
        }

        const bool medium = m_buttonSize == CommandTabButtonSizeKind::Medium;
        const int iconBandWidth = compactIconBandWidth();
        const int textWidth = std::max(
            medium ? scaledPx(56) : scaledPx(50),
            (m_assignedWidth > 0 ? m_assignedWidth : (medium ? scaledPx(108) : scaledPx(90)))
                - scaledPx(8)
                - iconBandWidth
                - scaledPx(10)
                - scaledPx(8)
        );
        const QRect textBounds = metrics.boundingRect(
            QRect(0, 0, textWidth, 1000),
            Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
            m_displayText
        );
        const int width = std::max(
            m_assignedWidth > 0 ? m_assignedWidth : (medium ? scaledPx(108) : scaledPx(90)),
            scaledPx(8) + iconBandWidth + scaledPx(8) + textBounds.width() + scaledPx(8)
        );
        return QSize(width, buttonHeight);
    }

    QColor baseTextBackgroundColor() const
    {
        if (m_theme == nullptr) {
            return QColor(QStringLiteral("#1e2329"));
        }
        return blendColors(m_theme->panelCardBackground, m_theme->panelBodyBottom, 0.24);
    }

    QColor readableButtonTextColor(const QColor& background) const
    {
        if (m_theme == nullptr) {
            return QColor(QStringLiteral("#d7dde8"));
        }
        if (m_theme->forceTextColor && m_theme->buttonText.isValid()) {
            return m_theme->buttonText;
        }
        return ensureReadableTextColor(background, m_theme->buttonText, 4.5);
    }

    QColor disabledButtonTextColor(const QColor& background) const
    {
        if (m_theme == nullptr) {
            return QColor(QStringLiteral("#8a95a3"));
        }
        if (m_theme->forceTextColor && m_theme->buttonText.isValid()) {
            return m_theme->buttonText;
        }
        const QColor preferred = readableButtonTextColor(background);
        const QColor softened = blendColors(
            preferred,
            m_theme->shellBackground,
            m_theme->isDark ? 0.16 : 0.30
        );
        return ensureReadableTextColor(background, softened, 4.0);
    }

    void paintGlassSpecularLayer(QPainter* painter, const QRect& surfaceRect, int cornerRadius) const
    {
        if (painter == nullptr || m_theme == nullptr || !isEnabled() || !surfaceRect.isValid()) {
            return;
        }

        const bool emphasized = m_hovered || m_pressed || hasFocus() || m_checked;
        painter->save();

        QPainterPath clipPath;
        clipPath.addRoundedRect(
            QRectF(surfaceRect),
            std::max(1, cornerRadius),
            std::max(1, cornerRadius)
        );
        painter->setClipPath(clipPath);
        painter->setPen(Qt::NoPen);

        // Top sheen: gives the "wet glass" cap highlight.
        const QRectF topSheenRect = QRectF(surfaceRect).adjusted(
            scaledPx(1),
            scaledPx(1),
            -scaledPx(1),
            -std::max(scaledPx(8), qRound(surfaceRect.height() * 0.52))
        );
        if (topSheenRect.height() > scaledPx(2)) {
            QLinearGradient topSheen(topSheenRect.topLeft(), topSheenRect.bottomLeft());
            topSheen.setColorAt(
                0.0,
                withAlpha(
                    blendColors(
                        m_theme->tabAccent,
                        QColor(QStringLiteral("#ffffff")),
                        m_theme->isDark ? 0.76 : 0.86
                    ),
                    emphasized ? (m_theme->isDark ? 78 : 112) : (m_theme->isDark ? 54 : 86)
                )
            );
            topSheen.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
            painter->setBrush(topSheen);
            painter->drawRoundedRect(topSheenRect, std::max(1, cornerRadius - scaledPx(1)), std::max(1, cornerRadius - scaledPx(1)));
        }

        // Diagonal streak: signature iOS-like moving light impression.
        QLinearGradient streak(
            QPointF(surfaceRect.left() - scaledPx(4), surfaceRect.top() + scaledPx(1)),
            QPointF(surfaceRect.right() + scaledPx(4), surfaceRect.bottom() - scaledPx(1))
        );
        streak.setColorAt(0.00, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        streak.setColorAt(0.24, withAlpha(QColor(QStringLiteral("#ffffff")), emphasized ? 34 : 20));
        streak.setColorAt(0.48, withAlpha(QColor(QStringLiteral("#ffffff")), emphasized ? 16 : 10));
        streak.setColorAt(0.72, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter->setBrush(streak);
        painter->drawRect(surfaceRect);

        // Localized glint near the top-left corner.
        const qreal glintRadius = std::max<qreal>(scaledPx(12), surfaceRect.width() * 0.35);
        QRadialGradient glint(
            QPointF(surfaceRect.left() + scaledPx(7), surfaceRect.top() + scaledPx(4)),
            glintRadius
        );
        glint.setColorAt(
            0.0,
            withAlpha(
                blendColors(
                    m_theme->panelBodyAccent,
                    QColor(QStringLiteral("#ffffff")),
                    m_theme->isDark ? 0.74 : 0.84
                ),
                emphasized ? (m_theme->isDark ? 56 : 78) : (m_theme->isDark ? 36 : 58)
            )
        );
        glint.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter->setBrush(glint);
        painter->drawRoundedRect(surfaceRect, std::max(1, cornerRadius), std::max(1, cornerRadius));

        // Subtle lower darkening to reinforce the glass depth.
        const QRectF lowerRect = QRectF(surfaceRect).adjusted(
            scaledPx(1),
            std::max(scaledPx(6), qRound(surfaceRect.height() * 0.52)),
            -scaledPx(1),
            -scaledPx(1)
        );
        if (lowerRect.height() > scaledPx(2)) {
            QLinearGradient lowerTint(lowerRect.topLeft(), lowerRect.bottomLeft());
            lowerTint.setColorAt(0.0, withAlpha(QColor(QStringLiteral("#000000")), 0));
            lowerTint.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#000000")), emphasized ? 34 : 24));
            painter->setBrush(lowerTint);
            painter->drawRoundedRect(lowerRect, std::max(1, cornerRadius - scaledPx(1)), std::max(1, cornerRadius - scaledPx(1)));
        }

        painter->restore();
    }

    void paintLargeButton(QPainter* painter)
    {
        const QRect contentRect = rect().adjusted(
            scaledPx(7), scaledPx(4), -scaledPx(7), -scaledPx(4)
        );
        const int reservedTextHeight = commandtabReservedTextHeight(fontMetrics());
        const int maxIconAreaHeight = m_textVisible
            ? std::max(scaledPx(24), contentRect.height() - reservedTextHeight - scaledPx(2))
            : contentRect.height();
        const int desiredIconEdge = m_textVisible
            ? std::clamp(m_iconOnlySize + scaledPx(8), scaledPx(24), scaledPx(56))
            : m_iconOnlySize;
        const int maxIconEdge = std::max(
            scaledPx(16),
            std::min(contentRect.width() - scaledPx(6), maxIconAreaHeight - scaledPx(6))
        );
        const int iconEdge = std::clamp(desiredIconEdge, scaledPx(16), maxIconEdge);
        const QSize iconSize(iconEdge, iconEdge);
        const QRect surfaceRect = rect().adjusted(
            scaledPx(1), scaledPx(1), -scaledPx(1), -scaledPx(1)
        );
        QColor textBackground = baseTextBackgroundColor();
        if (m_checked && !(m_pressed || m_hovered || hasFocus())) {
            const QColor fill = withAlpha(
                blendColors(m_theme->panelBodyTop, m_theme->tabAccent, 0.18),
                110
            );
            painter->setPen(QPen(withAlpha(m_theme->tabAccent, 140), 1.0));
            painter->setBrush(fill);
            painter->drawRoundedRect(surfaceRect, scaledPx(5), scaledPx(5));
            textBackground = blendColors(textBackground, fill, 0.24);
        }
        if (m_pressed || m_hovered || hasFocus()) {
            QColor fill = withAlpha(
                blendColors(m_theme->panelBodyTop, m_theme->quickHoverBackground, m_pressed ? 0.30 : 0.16),
                m_pressed ? 220 : 154
            );
            QColor border = m_pressed
                ? withAlpha(m_theme->buttonActiveBorder, 208)
                : withAlpha(m_theme->buttonBorder, 156);
            if (hasFocus() && !m_pressed) {
                border = m_theme->buttonFocusBorder;
            }
            painter->setPen(QPen(border, hasFocus() ? 1.4 * displayScaleFactor() : 1.0));
            painter->setBrush(fill);
            painter->drawRoundedRect(surfaceRect, scaledPx(5), scaledPx(5));
            textBackground = blendColors(textBackground, fill, m_pressed ? 0.46 : 0.30);
        }
        if (m_checked) {
            const QColor accentLine = withAlpha(m_theme->tabAccent, 210);
            painter->setPen(QPen(accentLine, scaledPx(2) * displayScaleFactor(), Qt::SolidLine, Qt::RoundCap));
            painter->setBrush(Qt::NoBrush);
            const int lineY = surfaceRect.bottom() - scaledPx(1);
            painter->drawLine(surfaceRect.left() + scaledPx(4), lineY, surfaceRect.right() - scaledPx(4), lineY);
        }
        paintGlassSpecularLayer(painter, surfaceRect, scaledPx(5));
        const int desiredPlateEdge = m_textVisible
            ? std::max(scaledPx(42), iconEdge + scaledPx(6))
            : (iconEdge + scaledPx(6));
        const int plateEdge = std::clamp(
            desiredPlateEdge,
            iconEdge + scaledPx(4),
            std::min(contentRect.width(), maxIconAreaHeight)
        );
        const QRect iconPlateRect = m_textVisible
            ? QRect(contentRect.left(), contentRect.top(), plateEdge, plateEdge)
            : QRect(
                contentRect.left() + std::max(0, (contentRect.width() - plateEdge) / 2),
                contentRect.top() + std::max(0, (contentRect.height() - plateEdge) / 2),
                plateEdge,
                plateEdge
            );
        QRect iconRect(
            iconPlateRect.left() + (iconPlateRect.width() - iconSize.width()) / 2,
            iconPlateRect.top() + (iconPlateRect.height() - iconSize.height()) / 2,
            iconSize.width(),
            iconSize.height()
        );
        if (m_hovered || m_pressed) {
            const QColor plateColor = withAlpha(
                blendColors(m_theme->buttonPlate, m_theme->shellBackground, 0.68),
                m_pressed ? 42 : 24
            );
            painter->setPen(Qt::NoPen);
            painter->setBrush(plateColor);
            painter->drawRoundedRect(
                iconPlateRect.adjusted(0, 0, -scaledPx(1), -scaledPx(1)),
                scaledPx(4),
                scaledPx(4)
            );
        }
        if (!m_icon.isNull()) {
            m_icon.paint(painter, iconRect, Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled);
        }
        if (hasMenuCommands()) {
            paintDropdownIndicator(
                painter,
                QRect(
                    contentRect.right() - scaledPx(12),
                    contentRect.top() + scaledPx(6),
                    scaledPx(8),
                    scaledPx(8)
                )
            );
        }

        if (!m_textVisible || m_textLines.isEmpty()) {
            return;
        }

        painter->setPen(isEnabled() ? readableButtonTextColor(textBackground) : disabledButtonTextColor(textBackground));
        QRect textRect(
            contentRect.left(),
            contentRect.bottom() - reservedTextHeight,
            contentRect.width(),
            reservedTextHeight
        );
        painter->drawText(
            textRect,
            Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
            m_textLines.join(QStringLiteral("\n"))
        );
    }

    void paintCompactButton(QPainter* painter)
    {
        const QRect contentRect = m_textVisible
            ? rect().adjusted(scaledPx(4), scaledPx(2), -scaledPx(4), -scaledPx(2))
            : rect();
        const bool medium = m_buttonSize == CommandTabButtonSizeKind::Medium;
        const int iconBandWidth = compactIconBandWidth();
        const int iconEdge = compactIconEdge();
        const int reservedTextHeight = commandtabReservedTextHeight(fontMetrics());
        const QRect surfaceRect = rect().adjusted(
            scaledPx(1), scaledPx(1), -scaledPx(1), -scaledPx(1)
        );
        QColor textBackground = baseTextBackgroundColor();
        if (m_checked && !(m_pressed || m_hovered || hasFocus())) {
            const QColor fill = withAlpha(
                blendColors(m_theme->panelBodyTop, m_theme->tabAccent, 0.16),
                100
            );
            painter->setPen(QPen(withAlpha(m_theme->tabAccent, 130), 1.0));
            painter->setBrush(fill);
            painter->drawRoundedRect(surfaceRect, scaledPx(5), scaledPx(5));
            textBackground = blendColors(textBackground, fill, 0.20);
        }
        if (m_pressed || m_hovered || hasFocus()) {
            QColor fill = withAlpha(
                blendColors(m_theme->panelBodyTop, m_theme->quickHoverBackground, m_pressed ? 0.28 : 0.14),
                m_pressed ? 214 : 146
            );
            QColor border = m_pressed
                ? withAlpha(m_theme->buttonActiveBorder, 204)
                : withAlpha(m_theme->buttonBorder, 148);
            if (hasFocus() && !m_pressed) {
                border = m_theme->buttonFocusBorder;
            }
            painter->setPen(QPen(border, hasFocus() ? 1.4 * displayScaleFactor() : 1.0));
            painter->setBrush(fill);
            painter->drawRoundedRect(surfaceRect, scaledPx(5), scaledPx(5));
            textBackground = blendColors(textBackground, fill, m_pressed ? 0.44 : 0.28);
        }
        if (m_checked) {
            const QColor accentLine = withAlpha(m_theme->tabAccent, 200);
            painter->setPen(QPen(accentLine, scaledPx(2) * displayScaleFactor(), Qt::SolidLine, Qt::RoundCap));
            painter->setBrush(Qt::NoBrush);
            const int lineY = surfaceRect.bottom() - scaledPx(1);
            painter->drawLine(surfaceRect.left() + scaledPx(3), lineY, surfaceRect.right() - scaledPx(3), lineY);
        }
        paintGlassSpecularLayer(painter, surfaceRect, scaledPx(5));
        const int iconPlateLeft = m_textVisible
            ? contentRect.left()
            : contentRect.left() + std::max(0, (contentRect.width() - iconBandWidth) / 2);
        const int iconPlateTop = m_textVisible
            ? (contentRect.top() + (medium ? scaledPx(1) : 0))
            : (contentRect.top() + std::max(0, (contentRect.height() - iconBandWidth) / 2));
        const QRect iconPlateRect(iconPlateLeft, iconPlateTop, iconBandWidth, iconBandWidth);
        QRect iconRect(
            iconPlateRect.left() + (iconPlateRect.width() - iconEdge) / 2,
            iconPlateRect.top() + (iconPlateRect.height() - iconEdge) / 2,
            iconEdge,
            iconEdge
        );
        if (m_hovered || m_pressed) {
            const QColor plateColor = withAlpha(
                blendColors(m_theme->buttonPlate, m_theme->shellBackground, 0.74),
                m_pressed ? 34 : 18
            );
            painter->setPen(Qt::NoPen);
            painter->setBrush(plateColor);
            painter->drawRoundedRect(
                iconPlateRect.adjusted(0, 0, -scaledPx(1), -scaledPx(1)),
                scaledPx(4),
                scaledPx(4)
            );
        }
        if (!m_icon.isNull()) {
            m_icon.paint(painter, iconRect, Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled);
        }
        if (hasMenuCommands()) {
            const QRect dropdownRect = m_textVisible
                ? QRect(
                    contentRect.right() - scaledPx(10),
                    contentRect.top() + scaledPx(4),
                    scaledPx(8),
                    scaledPx(8)
                )
                : QRect(
                    contentRect.right() - scaledPx(5),
                    contentRect.top() + scaledPx(1),
                    scaledPx(4),
                    scaledPx(4)
                );
            paintDropdownIndicator(painter, dropdownRect);
        }

        if (m_textLines.isEmpty()) {
            return;
        }

        painter->setPen(isEnabled() ? readableButtonTextColor(textBackground) : disabledButtonTextColor(textBackground));
        const int textTop = contentRect.top() + std::max(0, (contentRect.height() - reservedTextHeight) / 2);
        QRect textRect(
            contentRect.left() + iconPlateRect.width() + scaledPx(8),
            textTop,
            std::max(scaledPx(36), contentRect.width() - iconPlateRect.width() - scaledPx(10)),
            reservedTextHeight
        );
        painter->drawText(
            textRect,
            Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
            m_displayText
        );
    }

    void paintDropdownIndicator(QPainter* painter, const QRect& rect) const
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        QColor textBackground = baseTextBackgroundColor();
        if (m_pressed || m_hovered || hasFocus()) {
            const QColor fill = withAlpha(
                blendColors(m_theme->panelBodyTop, m_theme->quickHoverBackground, m_pressed ? 0.28 : 0.14),
                m_pressed ? 214 : 146
            );
            textBackground = blendColors(textBackground, fill, m_pressed ? 0.44 : 0.28);
        }
        painter->setBrush(
            isEnabled()
                ? readableButtonTextColor(textBackground)
                : disabledButtonTextColor(textBackground)
        );
        QPolygon triangle;
        triangle << QPoint(rect.left(), rect.top() + scaledPx(2))
                 << QPoint(rect.right(), rect.top() + scaledPx(2))
                 << QPoint(rect.center().x(), rect.bottom());
        painter->drawPolygon(triangle);
        painter->restore();
    }

    void applyCommandPopupMenuTheme(QMenu* menu) const
    {
        if (menu == nullptr || m_theme == nullptr) {
            return;
        }

        menu->setWindowFlag(Qt::FramelessWindowHint, true);
        menu->setAttribute(Qt::WA_TranslucentBackground, true);
        menu->setAttribute(Qt::WA_StyledBackground, true);

        const QColor menuSurface = withAlpha(
            blendColors(m_theme->panelCardBackground, m_theme->panelFooterBackground, 0.22),
            m_theme->isDark ? 204 : 224
        );
        const QColor menuHover = withAlpha(
            blendColors(m_theme->panelFooterBackground, m_theme->panelCardBackground, 0.38),
            m_theme->isDark ? 186 : 210
        );
        const QColor menuBorder = withAlpha(
            blendColors(m_theme->panelCardBorder, m_theme->shellBorder, 0.20),
            224
        );
        const QColor menuText = (m_theme->forceTextColor && m_theme->buttonText.isValid())
            ? m_theme->buttonText
            : ensureReadableTextColor(menuSurface, m_theme->buttonText, 4.8);
        const QColor disabledText = (m_theme->forceTextColor && m_theme->buttonText.isValid())
            ? m_theme->buttonText
            : ensureReadableTextColor(
                menuSurface,
                blendColors(
                    menuText,
                    m_theme->shellBackground,
                    m_theme->isDark ? 0.30 : 0.42
                ),
                4.2
            );
        const QColor focusBorder = withAlpha(m_theme->buttonFocusBorder, 232);

        QPalette menuPalette = menu->palette();
        const QPalette::ColorGroup activeGroups[] = {
            QPalette::Active,
            QPalette::Inactive,
        };
        for (QPalette::ColorGroup group : activeGroups) {
            menuPalette.setColor(group, QPalette::Window, menuSurface);
            menuPalette.setColor(group, QPalette::Base, menuSurface);
            menuPalette.setColor(group, QPalette::Button, menuSurface);
            menuPalette.setColor(group, QPalette::WindowText, menuText);
            menuPalette.setColor(group, QPalette::Text, menuText);
            menuPalette.setColor(group, QPalette::ButtonText, menuText);
            menuPalette.setColor(group, QPalette::Highlight, menuHover);
            menuPalette.setColor(group, QPalette::HighlightedText, menuText);
        }
        menuPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
        menu->setPalette(menuPalette);

        menu->setStyleSheet(
            QStringLiteral(R"(
QMenu {
    background: %1;
    border: 1px solid %2;
    border-radius: 10px;
    padding: 6px;
    margin: 0px;
}
QMenu::item {
    color: %3;
    background: transparent;
    padding: 6px 10px;
    margin: 1px 3px;
    border: 1px solid transparent;
    border-radius: 7px;
}
QMenu::item:selected {
    background: %4;
    border: 1px solid %5;
}
QMenu::item:disabled {
    color: %6;
}
QMenu::separator {
    height: 6px;
    margin: 0px;
    background: transparent;
}
)")
                .arg(menuSurface.name(QColor::HexArgb))
                .arg(menuBorder.name(QColor::HexArgb))
                .arg(menuText.name(QColor::HexArgb))
                .arg(menuHover.name(QColor::HexArgb))
                .arg(focusBorder.name(QColor::HexArgb))
                .arg(disabledText.name(QColor::HexArgb))
        );
    }

    void showMenu()
    {
        if (m_menuCommands.isEmpty()) {
            return;
        }
        QMenu menu(this);
        applyCommandPopupMenuTheme(&menu);
        for (const auto& command : m_menuCommands) {
            if (command.id.isEmpty()) {
                continue;
            }
            QAction* action = menu.addAction(commandtabCommandDisplayText(command));
            QAction* sourceAction = m_actionResolver ? m_actionResolver(command.id) : nullptr;
            QIcon actionIcon = loadCommandEntryIcon(command);
            if (sourceAction != nullptr && !sourceAction->icon().isNull()) {
                actionIcon = sourceAction->icon();
            }
            if (!actionIcon.isNull()) {
                action->setIcon(actionIcon);
            }
            if (sourceAction != nullptr) {
                action->setEnabled(sourceAction->isEnabled());
                QPointer<QAction> guardedAction(action);
                QPointer<QAction> guardedSource(sourceAction);
                connect(sourceAction, &QAction::changed, &menu, [guardedAction, guardedSource]() {
                    if (guardedAction.isNull()) {
                        return;
                    }
                    if (guardedSource.isNull()) {
                        guardedAction->setEnabled(true);
                        return;
                    }
                    guardedAction->setEnabled(guardedSource->isEnabled());
                });
            } else {
                action->setEnabled(true);
            }
            connect(action, &QAction::triggered, this, [this, command]() {
                if (m_menuCommandHandler) {
                    m_menuCommandHandler(command.id);
                }
            });
        }
        if (menu.isEmpty()) {
            return;
        }
        menu.ensurePolished();
        menu.adjustSize();
        QPainterPath clipPath;
        clipPath.addRoundedRect(
            QRectF(menu.rect().adjusted(0, 0, -1, -1)),
            scaledPx(10),
            scaledPx(10)
        );
        menu.setMask(QRegion(clipPath.toFillPolygon().toPolygon()));
        menu.exec(mapToGlobal(rect().bottomLeft()));
    }

    const CommandTabModel::CommandTabTheme* m_theme = nullptr;
    QIcon m_icon;
    QString m_commandId;
    QString m_rawText;
    QString m_displayText;
    QStringList m_textLines;
    QSize m_preferredSize;
    std::function<void()> m_triggeredHandler;
    std::function<void(const QString&)> m_menuCommandHandler;
    std::function<void(const QString&)> m_contextCommandHandler;
    std::function<QAction*(const QString&)> m_actionResolver;
    QVector<CommandTabCommandEntry> m_menuCommands;
    int m_assignedWidth = 0;
    int m_iconOnlySize = 20;
    int m_compactButtonPadding = 3;
    int m_displayScalePercent = 100;
    CommandTabButtonSizeKind m_buttonSize = CommandTabButtonSizeKind::Small;
    bool m_textVisible = true;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_checked = false;
    bool m_inQuickAccess = false;

    qreal displayScaleFactor() const
    {
        return static_cast<qreal>(std::clamp(m_displayScalePercent, 60, 140)) / 100.0;
    }

    int scaledPx(int value) const
    {
        return std::max(1, qRound(static_cast<qreal>(value) * displayScaleFactor()));
    }
};

class CommandTabSeparatorWidget final : public QWidget
{
public:
    explicit CommandTabSeparatorWidget(
        const CommandTabModel::CommandTabTheme* theme,
        int displayScalePercent = 100,
        QWidget* parent = nullptr
    )
        : QWidget(parent)
        , m_theme(theme)
        , m_displayScalePercent(std::clamp(displayScalePercent, 60, 140))
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    QSize sizeHint() const override
    {
        return QSize(scaledPx(8), scaledPx(100));
    }

    QSize minimumSizeHint() const override
    {
        return sizeHint();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect capsuleRect(
            rect().center().x() - scaledPx(1),
            scaledPx(8),
            scaledPx(3),
            std::max(0, height() - scaledPx(16))
        );
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(m_theme->separator, 168));
        painter.drawRoundedRect(
            capsuleRect,
            1.5 * displayScaleFactor(),
            1.5 * displayScaleFactor()
        );
    }

private:
    const CommandTabModel::CommandTabTheme* m_theme = nullptr;
    int m_displayScalePercent = 100;

    qreal displayScaleFactor() const
    {
        return static_cast<qreal>(std::clamp(m_displayScalePercent, 60, 140)) / 100.0;
    }

    int scaledPx(int value) const
    {
        return std::max(1, qRound(static_cast<qreal>(value) * displayScaleFactor()));
    }
};

class CommandTabPanelBodyWidget final : public QWidget
{
public:
    explicit CommandTabPanelBodyWidget(
        const CommandTabModel::CommandTabTheme* theme,
        const CommandTabSettingsState& settings,
        QWidget* parent = nullptr
    )
        : QWidget(parent)
        , m_theme(theme)
        , m_settingsState(settings)
    {
        setObjectName(QStringLiteral("CommandTabPanelBody"));
        setAttribute(Qt::WA_StyledBackground, true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void addCommandWidget(QWidget* widget)
    {
        if (widget == nullptr) {
            return;
        }
        widget->setParent(this);
        queueWidgetForLayout(widget);
    }

    void addSeparatorWidget(QWidget* widget)
    {
        if (widget == nullptr) {
            return;
        }
        widget->setParent(this);
        queueWidgetForLayout(widget);
    }

    void finalizeLayout()
    {
        applyButtonSettingsToWidgets(m_settingsState);
        flushPendingColumns();
        rebuildColumnsFromCurrentWidgets();
        recalculateMetrics();
        setFixedSize(m_preferredSize);
        layoutColumns();
    }

    void applySettings(const CommandTabSettingsState& settings)
    {
        m_settingsState = settings;
        applyButtonSettingsToWidgets(settings);

        rebuildColumnsFromCurrentWidgets();

        recalculateMetrics();
        setFixedSize(m_preferredSize);
        layoutColumns();
        updateGeometry();
        update();
    }

    QSize sizeHint() const override
    {
        return m_preferredSize;
    }

    QSize minimumSizeHint() const override
    {
        return m_preferredSize;
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        layoutColumns();
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect outerRect = rect().adjusted(
            scaledPx(1), scaledPx(1), -scaledPx(1), -scaledPx(1)
        );
        QLinearGradient gradient(outerRect.topLeft(), outerRect.bottomLeft());
        gradient.setColorAt(0.0, blendColors(m_theme->panelCardBackground, m_theme->panelBodyTop, 0.30));
        gradient.setColorAt(0.48, blendColors(m_theme->panelCardBackground, m_theme->shellBackground, 0.08));
        gradient.setColorAt(1.0, blendColors(m_theme->panelCardBackground, m_theme->panelBodyBottom, 0.22));
        painter.setPen(QPen(withAlpha(m_theme->panelCardBorder, 196), std::max(1.0, displayScaleFactor())));
        painter.setBrush(gradient);
        painter.drawRoundedRect(outerRect, scaledPx(8), scaledPx(8));

        // Glass-like specular highlight on the top area of each panel card.
        const QRectF glossRect = outerRect.adjusted(
            scaledPx(2),
            scaledPx(2),
            -scaledPx(2),
            -std::max(scaledPx(8), qRound(outerRect.height() * 0.42))
        );
        if (glossRect.height() > scaledPx(4)) {
            QLinearGradient glossGradient(glossRect.topLeft(), glossRect.bottomLeft());
            glossGradient.setColorAt(
                0.0,
                withAlpha(
                    blendColors(m_theme->panelBodyAccent, QColor(QStringLiteral("#ffffff")), m_theme->isDark ? 0.62 : 0.78),
                    m_theme->isDark ? 72 : 108
                )
            );
            glossGradient.setColorAt(
                1.0,
                withAlpha(QColor(QStringLiteral("#ffffff")), 0)
            );
            painter.setPen(Qt::NoPen);
            painter.setBrush(glossGradient);
            painter.drawRoundedRect(glossRect, scaledPx(7), scaledPx(7));
        }

        const qreal glowRadius = std::max<qreal>(scaledPx(44), outerRect.width() * 0.58);
        QRadialGradient leftGlow(
            QPointF(outerRect.left() + scaledPx(12), outerRect.top() + scaledPx(8)),
            glowRadius
        );
        leftGlow.setColorAt(
            0.0,
            withAlpha(
                blendColors(
                    m_theme->panelBodyAccent,
                    QColor(QStringLiteral("#ffffff")),
                    m_theme->isDark ? 0.70 : 0.82
                ),
                m_theme->isDark ? 58 : 82
            )
        );
        leftGlow.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(leftGlow);
        painter.drawRoundedRect(outerRect, scaledPx(8), scaledPx(8));

        const qreal rightGlowRadius = std::max<qreal>(scaledPx(30), outerRect.width() * 0.34);
        QRadialGradient rightGlow(
            QPointF(outerRect.right() - scaledPx(12), outerRect.top() + scaledPx(10)),
            rightGlowRadius
        );
        rightGlow.setColorAt(
            0.0,
            withAlpha(
                blendColors(
                    m_theme->panelBodyTop,
                    QColor(QStringLiteral("#ffffff")),
                    m_theme->isDark ? 0.54 : 0.70
                ),
                m_theme->isDark ? 34 : 54
            )
        );
        rightGlow.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter.setBrush(rightGlow);
        painter.drawRoundedRect(outerRect, scaledPx(8), scaledPx(8));

        const QRectF lowerTintRect = outerRect.adjusted(
            scaledPx(2),
            std::max(scaledPx(6), qRound(outerRect.height() * 0.42)),
            -scaledPx(2),
            -scaledPx(2)
        );
        if (lowerTintRect.height() > scaledPx(4)) {
            QLinearGradient lowerTint(lowerTintRect.topLeft(), lowerTintRect.bottomLeft());
            lowerTint.setColorAt(
                0.0,
                withAlpha(
                    blendColors(
                        m_theme->panelBodyBottom,
                        m_theme->shellBackground,
                        m_theme->isDark ? 0.22 : 0.14
                    ),
                    m_theme->isDark ? 20 : 16
                )
            );
            lowerTint.setColorAt(
                1.0,
                withAlpha(
                    blendColors(
                        m_theme->panelBodyBottom,
                        m_theme->shellBackground,
                        m_theme->isDark ? 0.42 : 0.30
                    ),
                    m_theme->isDark ? 56 : 44
                )
            );
            painter.setBrush(lowerTint);
            painter.drawRoundedRect(lowerTintRect, scaledPx(7), scaledPx(7));
        }

        painter.setPen(
            QPen(
                withAlpha(
                    blendColors(m_theme->panelCardBorder, QColor(QStringLiteral("#ffffff")), m_theme->isDark ? 0.30 : 0.46),
                    m_theme->isDark ? 74 : 104
                ),
                std::max(1.0, displayScaleFactor())
            )
        );
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(
            outerRect.adjusted(scaledPx(1), scaledPx(1), -scaledPx(1), -scaledPx(1)),
            scaledPx(7),
            scaledPx(7)
        );

        painter.setPen(
            QPen(
                withAlpha(
                    blendColors(m_theme->panelBodyAccent, QColor(QStringLiteral("#ffffff")), m_theme->isDark ? 0.46 : 0.64),
                    m_theme->isDark ? 104 : 136
                ),
                std::max(1.0, displayScaleFactor())
            )
        );
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(
            QPointF(outerRect.left() + scaledPx(4), outerRect.top() + scaledPx(2)),
            QPointF(outerRect.right() - scaledPx(4), outerRect.top() + scaledPx(2))
        );
    }

private:
    enum class PanelColumnType {
        Small,
        Medium,
        WideSmall,
        Large,
        Separator
    };

    struct PanelColumn
    {
        PanelColumnType type = PanelColumnType::Small;
        QVector<QWidget*> widgets;
        int width = 0;
    };

    enum class CompactColumnKind {
        SmallIconOnly,
        SmallText,
        MediumIconOnly,
        MediumText,
        LargeIconOnly,
        Large,
        Separator
    };

    bool compactPanelLayoutEnabled() const
    {
        return m_settingsState.compactPanelLayout;
    }

    int compactPanelSpacingValue() const
    {
        return std::clamp(m_settingsState.compactPanelSpacing, 0, 6);
    }

    qreal displayScaleFactor() const
    {
        return static_cast<qreal>(std::clamp(m_settingsState.commandtabScalePercent, 60, 140))
            / 100.0;
    }

    int scaledPx(int value) const
    {
        return std::max(1, qRound(static_cast<qreal>(value) * displayScaleFactor()));
    }

    void applyButtonSettingsToWidgets(const CommandTabSettingsState& settings)
    {
        auto applyToWidget = [&settings](QWidget* widget) {
            if (widget == nullptr) {
                return;
            }
            if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                commandtabButton->applySettings(settings);
            }
            const auto nestedWidgets = widget->findChildren<QWidget*>(
                QString(),
                Qt::FindChildrenRecursively
            );
            for (auto* nestedWidget : nestedWidgets) {
                auto* nestedButton = dynamic_cast<CommandTabCommandButton*>(nestedWidget);
                if (nestedButton != nullptr) {
                    nestedButton->applySettings(settings);
                }
            }
        };

        for (auto& column : m_columns) {
            for (auto* widget : column.widgets) {
                applyToWidget(widget);
            }
        }
        for (auto* widget : m_pendingSmallColumn) {
            applyToWidget(widget);
        }
        for (auto* widget : m_pendingMediumColumn) {
            applyToWidget(widget);
        }
    }

    bool pendingSmallHasTextButtons() const
    {
        for (auto* widget : m_pendingSmallColumn) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton != nullptr && commandtabButton->showsText()) {
                return true;
            }
        }
        return false;
    }

    bool pendingSmallHasIconOnlyButtons() const
    {
        for (auto* widget : m_pendingSmallColumn) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton != nullptr && !commandtabButton->showsText()) {
                return true;
            }
        }
        return false;
    }

    void flushPendingColumns()
    {
        flushPendingMediumColumn();
        flushPendingSmallColumn();
    }

    void queueWidgetForLayout(QWidget* widget, bool compactLayoutMode = false)
    {
        if (widget == nullptr) {
            return;
        }

        if (dynamic_cast<CommandTabSeparatorWidget*>(widget) != nullptr) {
            flushPendingColumns();
            PanelColumn column;
            column.type = PanelColumnType::Separator;
            column.widgets.push_back(widget);
            m_columns.push_back(column);
            return;
        }

        if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
            if (compactLayoutMode) {
                // In compact icon-only mode we normalize all command sizes to the
                // same dense 3-row packing so workbenches stay visually uniform.
                if (!commandtabButton->showsText()) {
                    flushPendingMediumColumn();
                    m_pendingSmallColumn.push_back(widget);
                    if (m_pendingSmallColumn.size() >= 3) {
                        flushPendingSmallColumn();
                    }
                    return;
                }
                if (commandtabButton->isLargeButton()) {
                    flushPendingColumns();
                    PanelColumn column;
                    column.type = PanelColumnType::Large;
                    column.widgets.push_back(widget);
                    m_columns.push_back(column);
                    return;
                }
                if (commandtabButton->isMediumButton()) {
                    flushPendingSmallColumn();
                    m_pendingMediumColumn.push_back(widget);
                    if (m_pendingMediumColumn.size() >= 2) {
                        flushPendingMediumColumn();
                    }
                    return;
                }
                flushPendingMediumColumn();
                m_pendingSmallColumn.push_back(widget);
                if (m_pendingSmallColumn.size() >= 3) {
                    flushPendingSmallColumn();
                }
                return;
            }

            if (!commandtabButton->showsText()) {
                flushPendingMediumColumn();
                if (pendingSmallHasTextButtons()) {
                    flushPendingSmallColumn();
                }
                m_pendingSmallColumn.push_back(widget);
                if (m_pendingSmallColumn.size() >= 3) {
                    flushPendingSmallColumn();
                }
                return;
            }

            if (commandtabButton->isLargeButton()) {
                flushPendingColumns();
                PanelColumn column;
                column.type = PanelColumnType::Large;
                column.widgets.push_back(widget);
                m_columns.push_back(column);
                return;
            }
            if (commandtabButton->isMediumButton()) {
                flushPendingSmallColumn();
                m_pendingMediumColumn.push_back(widget);
                if (m_pendingMediumColumn.size() >= 2) {
                    flushPendingMediumColumn();
                }
                return;
            }

            flushPendingMediumColumn();
            if (pendingSmallHasIconOnlyButtons()) {
                flushPendingSmallColumn();
            }
            m_pendingSmallColumn.push_back(widget);
            if (m_pendingSmallColumn.size() >= 3) {
                flushPendingSmallColumn();
            }
            return;
        }

        if (compactLayoutMode) {
            if (widget->property("commandtabColumnType").toString() == QStringLiteral("large")) {
                flushPendingColumns();
                PanelColumn column;
                column.type = PanelColumnType::Large;
                column.widgets.push_back(widget);
                m_columns.push_back(column);
                return;
            }
            flushPendingMediumColumn();
            m_pendingSmallColumn.push_back(widget);
            if (m_pendingSmallColumn.size() >= 3) {
                flushPendingSmallColumn();
            }
            return;
        }

        if (widget->property("commandtabColumnType").toString() == QStringLiteral("large")) {
            flushPendingColumns();
            PanelColumn column;
            column.type = PanelColumnType::Large;
            column.widgets.push_back(widget);
            m_columns.push_back(column);
            return;
        }

        flushPendingMediumColumn();
        if (pendingSmallHasIconOnlyButtons()) {
            flushPendingSmallColumn();
        }
        m_pendingSmallColumn.push_back(widget);
        if (m_pendingSmallColumn.size() >= 3) {
            flushPendingSmallColumn();
        }
    }

    void rebuildColumnsFromCurrentWidgets()
    {
        QVector<QWidget*> orderedWidgets;
        orderedWidgets.reserve(64);
        for (const auto& column : m_columns) {
            for (auto* widget : column.widgets) {
                if (widget != nullptr) {
                    orderedWidgets.push_back(widget);
                }
            }
        }

        bool sawCommandTabButton = false;
        bool hasVisibleText = false;
        for (auto* widget : orderedWidgets) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton == nullptr) {
                continue;
            }
            sawCommandTabButton = true;
            if (commandtabButton->showsText()) {
                hasVisibleText = true;
                break;
            }
        }
        m_iconOnlyPackingMode = sawCommandTabButton && !hasVisibleText;
        const bool compactLayoutMode = compactPanelLayoutEnabled();

        m_columns.clear();
        m_pendingSmallColumn.clear();
        m_pendingMediumColumn.clear();

        for (auto* widget : orderedWidgets) {
            queueWidgetForLayout(widget, compactLayoutMode);
        }
        flushPendingColumns();
    }

    int minColumnWidth(PanelColumnType type) const
    {
        if (!compactPanelLayoutEnabled()) {
            switch (type) {
                case PanelColumnType::Small:
                    return scaledPx(96);
                case PanelColumnType::Medium:
                    return scaledPx(108);
                case PanelColumnType::WideSmall:
                    return scaledPx(108);
                case PanelColumnType::Large:
                    return scaledPx(96);
                case PanelColumnType::Separator:
                    return scaledPx(8);
            }
            return scaledPx(96);
        }

        switch (type) {
            case PanelColumnType::Small:
                return scaledPx(78);
            case PanelColumnType::Medium:
                return scaledPx(96);
            case PanelColumnType::WideSmall:
                return scaledPx(96);
            case PanelColumnType::Large:
                return scaledPx(110);
            case PanelColumnType::Separator:
                return scaledPx(6);
        }
        return scaledPx(78);
    }

    int maxColumnWidth(PanelColumnType type) const
    {
        if (!compactPanelLayoutEnabled()) {
            switch (type) {
                case PanelColumnType::Small:
                    return scaledPx(222);
                case PanelColumnType::Medium:
                    return scaledPx(240);
                case PanelColumnType::WideSmall:
                    return scaledPx(252);
                case PanelColumnType::Large:
                    return scaledPx(228);
                case PanelColumnType::Separator:
                    return scaledPx(8);
            }
            return scaledPx(600);
        }

        switch (type) {
            case PanelColumnType::Small:
                return scaledPx(180);
            case PanelColumnType::Medium:
                return scaledPx(210);
            case PanelColumnType::WideSmall:
                return scaledPx(210);
            case PanelColumnType::Large:
                return scaledPx(260);
            case PanelColumnType::Separator:
                return scaledPx(8);
        }
        return scaledPx(260);
    }

    int normalizeColumnWidthForType(int width, PanelColumnType type) const
    {
        const int minimum = minColumnWidth(type);
        const int maximum = maxColumnWidth(type);
        int normalized = std::clamp(width, minimum, maximum);
        if (type != PanelColumnType::Separator && (normalized % 2) != 0) {
            normalized = std::min(maximum, normalized + 1);
        }
        return normalized;
    }

    bool columnUsesCompactWidth(const PanelColumn& column) const
    {
        bool sawCompactCandidate = false;
        for (auto* widget : column.widgets) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton != nullptr) {
                sawCompactCandidate = true;
                if (commandtabButton->showsText()) {
                    return false;
                }
                continue;
            }
            if (widget != nullptr && widget->property("commandtabCompactIconOnly").toBool()) {
                sawCompactCandidate = true;
                continue;
            }
            return false;
        }
        return sawCompactCandidate;
    }

    CompactColumnKind compactColumnKindForColumn(const PanelColumn& column) const
    {
        if (column.type == PanelColumnType::Separator) {
            return CompactColumnKind::Separator;
        }

        bool hasText = false;
        bool sawButton = false;
        bool sawCompactIconOnlyWidget = false;
        for (auto* widget : column.widgets) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton == nullptr) {
                if (widget != nullptr && widget->property("commandtabCompactIconOnly").toBool()) {
                    sawCompactIconOnlyWidget = true;
                }
                continue;
            }
            sawButton = true;
            if (commandtabButton->showsText()) {
                hasText = true;
                break;
            }
        }
        if (column.type == PanelColumnType::Large) {
            if (!hasText && (sawButton || sawCompactIconOnlyWidget)) {
                return CompactColumnKind::LargeIconOnly;
            }
            return CompactColumnKind::Large;
        }
        if (column.type == PanelColumnType::Medium) {
            if (!hasText && (sawButton || sawCompactIconOnlyWidget)) {
                return CompactColumnKind::MediumIconOnly;
            }
            return hasText ? CompactColumnKind::MediumText : CompactColumnKind::MediumIconOnly;
        }
        if (column.type == PanelColumnType::Small || column.type == PanelColumnType::WideSmall) {
            if (!hasText && (sawButton || sawCompactIconOnlyWidget)) {
                return CompactColumnKind::SmallIconOnly;
            }
            return CompactColumnKind::SmallText;
        }
        return CompactColumnKind::SmallText;
    }

    int normalizeCompactColumnWidth(int desiredWidth, CompactColumnKind kind, bool compactMode) const
    {
        if (!compactMode) {
            switch (kind) {
                case CompactColumnKind::Separator:
                    return normalizeColumnWidthForType(desiredWidth, PanelColumnType::Separator);
                case CompactColumnKind::Large:
                case CompactColumnKind::LargeIconOnly:
                    return normalizeColumnWidthForType(desiredWidth, PanelColumnType::Large);
                case CompactColumnKind::MediumIconOnly:
                case CompactColumnKind::MediumText:
                    return normalizeColumnWidthForType(desiredWidth, PanelColumnType::Medium);
                case CompactColumnKind::SmallIconOnly:
                case CompactColumnKind::SmallText:
                default:
                    return normalizeColumnWidthForType(desiredWidth, PanelColumnType::Small);
            }
        }

        // Compact mode keeps uniform visual rhythm with snapped width steps,
        // while still expanding when content/icon size requires it.
        const auto snapCompactWidth = [](int value, int minimum, int maximum, int step) -> int {
            const int clamped = std::clamp(value, minimum, maximum);
            const int offset = std::max(0, clamped - minimum);
            const int snapped = minimum + ((offset + step - 1) / step) * step;
            return std::clamp(snapped, minimum, maximum);
        };
        switch (kind) {
            case CompactColumnKind::SmallIconOnly:
                return snapCompactWidth(
                    std::max(scaledPx(34), desiredWidth),
                    scaledPx(34),
                    scaledPx(96),
                    std::max(1, scaledPx(2))
                );
            case CompactColumnKind::SmallText:
                return snapCompactWidth(
                    std::max(scaledPx(90), desiredWidth),
                    scaledPx(90),
                    scaledPx(114),
                    std::max(1, scaledPx(6))
                );
            case CompactColumnKind::MediumIconOnly:
                return snapCompactWidth(
                    std::max(scaledPx(36), desiredWidth),
                    scaledPx(36),
                    scaledPx(104),
                    std::max(1, scaledPx(2))
                );
            case CompactColumnKind::MediumText:
                return snapCompactWidth(
                    std::max(scaledPx(108), desiredWidth),
                    scaledPx(108),
                    scaledPx(132),
                    std::max(1, scaledPx(6))
                );
            case CompactColumnKind::LargeIconOnly:
                return snapCompactWidth(
                    std::max(scaledPx(40), desiredWidth),
                    scaledPx(40),
                    scaledPx(116),
                    std::max(1, scaledPx(2))
                );
            case CompactColumnKind::Large:
                return snapCompactWidth(
                    std::max(scaledPx(126), desiredWidth),
                    scaledPx(126),
                    scaledPx(162),
                    std::max(1, scaledPx(6))
                );
            case CompactColumnKind::Separator:
            default:
                return scaledPx(6);
        }
    }

    int normalizeColumnWidthForColumn(int width, const PanelColumn& column) const
    {
        if (!compactPanelLayoutEnabled()) {
            return normalizeColumnWidthForType(width, column.type);
        }
        return normalizeCompactColumnWidth(width, compactColumnKindForColumn(column), true);
    }

    bool shouldUseCompactPanelDensity() const
    {
        if (compactPanelLayoutEnabled()) {
            return true;
        }
        bool sawCommandTabButton = false;
        for (const auto& column : m_columns) {
            for (auto* widget : column.widgets) {
                auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
                if (commandtabButton == nullptr) {
                    continue;
                }
                sawCommandTabButton = true;
                if (commandtabButton->showsText()) {
                    return false;
                }
            }
        }
        return sawCommandTabButton;
    }

    void flushPendingSmallColumn()
    {
        if (m_pendingSmallColumn.isEmpty()) {
            return;
        }
        PanelColumn column;
        if (compactPanelLayoutEnabled() || m_iconOnlyPackingMode) {
            column.type = PanelColumnType::Small;
            column.widgets = m_pendingSmallColumn;
            m_columns.push_back(column);
            m_pendingSmallColumn.clear();
            return;
        }
        bool prefersWideLayout = false;
        for (auto* widget : m_pendingSmallColumn) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton != nullptr && commandtabButton->prefersWideSmallLayout()) {
                prefersWideLayout = true;
                break;
            }
        }
        column.type = prefersWideLayout ? PanelColumnType::WideSmall : PanelColumnType::Small;
        column.widgets = m_pendingSmallColumn;
        m_columns.push_back(column);
        m_pendingSmallColumn.clear();
    }

    void flushPendingMediumColumn()
    {
        if (m_pendingMediumColumn.isEmpty()) {
            return;
        }
        PanelColumn column;
        column.type = PanelColumnType::Medium;
        column.widgets = m_pendingMediumColumn;
        m_columns.push_back(column);
        m_pendingMediumColumn.clear();
    }

    void updateSmallColumnType(PanelColumn& column)
    {
        if (column.type != PanelColumnType::Small && column.type != PanelColumnType::WideSmall) {
            return;
        }
        if (compactPanelLayoutEnabled()) {
            column.type = PanelColumnType::Small;
            return;
        }

        bool prefersWideLayout = false;
        for (auto* widget : column.widgets) {
            auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget);
            if (commandtabButton != nullptr && commandtabButton->prefersWideSmallLayout()) {
                prefersWideLayout = true;
                break;
            }
        }
        column.type = prefersWideLayout ? PanelColumnType::WideSmall : PanelColumnType::Small;
    }

    void recalculateMetrics()
    {
        const bool compactDensity = shouldUseCompactPanelDensity();
        if (compactPanelLayoutEnabled()) {
            const int compactSpacing = compactPanelSpacingValue();
            m_margins = QMargins(scaledPx(2), scaledPx(4), scaledPx(2), scaledPx(4));
            m_horizontalSpacing = std::max(1, scaledPx(compactSpacing));
            m_verticalSpacing = std::max(1, scaledPx(std::max(0, compactSpacing - 1)));
        } else if (m_iconOnlyPackingMode) {
            m_margins = QMargins(scaledPx(2), scaledPx(4), scaledPx(2), scaledPx(4));
            m_horizontalSpacing = 0;
            m_verticalSpacing = scaledPx(1);
        } else {
            m_margins = compactDensity
                ? QMargins(scaledPx(4), scaledPx(4), scaledPx(4), scaledPx(4))
                : QMargins(scaledPx(6), scaledPx(4), scaledPx(6), scaledPx(4));
            m_horizontalSpacing = compactDensity ? scaledPx(1) : scaledPx(2);
            m_verticalSpacing = scaledPx(2);
        }

        const QFontMetrics metrics(font());
        const int reservedTextHeight = commandtabReservedTextHeight(metrics);
        const int standardRowHeight = std::max(scaledPx(48), reservedTextHeight + scaledPx(8));

        m_contentHeight = standardRowHeight * 3 + m_verticalSpacing * 2;
        {
            const int rows = 3;
            const int totalSpacing = m_verticalSpacing * (rows - 1);
            const int baseRowHeight = std::max(scaledPx(28), (m_contentHeight - totalSpacing) / rows);
            m_rowHeights = QVector<int>(rows, baseRowHeight);
            const int usedHeight = baseRowHeight * rows + totalSpacing;
            if (!m_rowHeights.isEmpty() && usedHeight < m_contentHeight) {
                m_rowHeights.back() += (m_contentHeight - usedHeight);
            }
        }
        m_mediumRowHeight = std::max(scaledPx(30), (m_contentHeight - m_verticalSpacing) / 2);

        int totalColumnWidth = 0;
        int columnCount = 0;
        for (auto& column : m_columns) {
            if (column.type == PanelColumnType::Separator) {
                column.width = compactPanelLayoutEnabled() ? scaledPx(6) : scaledPx(8);
            } else if (column.type == PanelColumnType::Large) {
                const bool iconOnlyColumn = compactPanelLayoutEnabled() && columnUsesCompactWidth(column);
                int desiredWidth = iconOnlyColumn ? scaledPx(40) : scaledPx(126);
                for (auto* widget : column.widgets) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                        desiredWidth = std::max(desiredWidth, commandtabButton->idealPanelWidth());
                    } else if (widget != nullptr) {
                        desiredWidth = std::max(desiredWidth, widget->sizeHint().width());
                    }
                }
                column.width = normalizeColumnWidthForColumn(desiredWidth, column);
                if (!column.widgets.isEmpty()) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(column.widgets.first())) {
                        commandtabButton->applyPanelWidth(column.width);
                    }
                }
            } else if (column.type == PanelColumnType::Medium) {
                const bool iconOnlyColumn = compactPanelLayoutEnabled() && columnUsesCompactWidth(column);
                int desiredWidth = iconOnlyColumn ? scaledPx(36) : scaledPx(108);
                for (auto* widget : column.widgets) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                        desiredWidth = std::max(desiredWidth, commandtabButton->idealPanelWidth());
                    } else if (widget != nullptr) {
                        desiredWidth = std::max(desiredWidth, widget->sizeHint().width());
                    }
                }
                column.width = normalizeColumnWidthForColumn(desiredWidth, column);
                for (auto* widget : column.widgets) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                        commandtabButton->applyPanelWidth(column.width);
                    }
                }
            } else {
                updateSmallColumnType(column);
                const bool iconOnlyColumn = compactPanelLayoutEnabled() && columnUsesCompactWidth(column);
                int desiredWidth = iconOnlyColumn ? scaledPx(34) : scaledPx(90);
                for (auto* widget : column.widgets) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                        desiredWidth = std::max(desiredWidth, commandtabButton->idealPanelWidth());
                    } else if (widget != nullptr) {
                        desiredWidth = std::max(desiredWidth, widget->sizeHint().width());
                    }
                }
                column.width = normalizeColumnWidthForColumn(desiredWidth, column);
                const int rowCount = std::min(3, static_cast<int>(column.widgets.size()));
                for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
                    if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(column.widgets.at(rowIndex))) {
                        commandtabButton->applyPanelWidth(column.width);
                    }
                }
            }
            totalColumnWidth += column.width;
            ++columnCount;
        }

        const int spacingWidth = std::max(0, columnCount - 1) * m_horizontalSpacing;
        m_preferredSize = QSize(
            m_margins.left() + totalColumnWidth + spacingWidth + m_margins.right(),
            m_margins.top() + m_contentHeight + m_margins.bottom()
        );
    }

    void layoutColumns()
    {
        if (m_columns.isEmpty()) {
            return;
        }

        auto layoutCellWidget = [this](
                                    QWidget* widget,
                                    int cellX,
                                    int cellY,
                                    int cellWidth,
                                    int cellHeight
                                ) {
            if (widget == nullptr) {
                return;
            }

            int widgetX = cellX;
            int widgetY = cellY;
            int widgetWidth = cellWidth;
            int widgetHeight = cellHeight;
            if (auto* commandtabButton = dynamic_cast<CommandTabCommandButton*>(widget)) {
                if (!commandtabButton->showsText()) {
                    const int idealEdge = commandtabButton->idealPanelWidth();
                    const int squareEdge = std::max(
                        scaledPx(18),
                        std::min({idealEdge, cellWidth, cellHeight})
                    );
                    widgetWidth = squareEdge;
                    widgetHeight = squareEdge;
                    widgetX = cellX + std::max(0, (cellWidth - squareEdge) / 2);
                    widgetY = cellY + std::max(0, (cellHeight - squareEdge) / 2);
                }
            }

            widget->setGeometry(widgetX, widgetY, widgetWidth, widgetHeight);
            widget->show();
        };

        int x = m_margins.left();
        for (int columnIndex = 0; columnIndex < m_columns.size(); ++columnIndex) {
            auto& column = m_columns[columnIndex];
            if (column.type == PanelColumnType::Separator) {
                if (!column.widgets.isEmpty()) {
                    column.widgets.first()->setGeometry(x, m_margins.top(), column.width, m_contentHeight);
                    column.widgets.first()->show();
                }
            } else if (column.type == PanelColumnType::Large) {
                if (!column.widgets.isEmpty()) {
                    layoutCellWidget(column.widgets.first(), x, m_margins.top(), column.width, m_contentHeight);
                }
            } else if (column.type == PanelColumnType::WideSmall) {
                int currentY = m_margins.top();
                for (auto* widget : column.widgets) {
                    layoutCellWidget(widget, x, currentY, column.width, m_rowHeights.at(0));
                    currentY += m_rowHeights.at(0) + m_verticalSpacing;
                }
            } else if (column.type == PanelColumnType::Medium) {
                int currentY = m_margins.top();
                for (auto* widget : column.widgets) {
                    layoutCellWidget(widget, x, currentY, column.width, m_mediumRowHeight);
                    currentY += m_mediumRowHeight + m_verticalSpacing;
                }
            } else {
                const qsizetype rowCount = std::min(column.widgets.size(), m_rowHeights.size());
                int startY = m_margins.top();
                if (m_iconOnlyPackingMode && rowCount > 0) {
                    const int cellH = m_rowHeights.at(0);
                    const int totalRowHeight = static_cast<int>(rowCount) * cellH
                        + std::max(0, static_cast<int>(rowCount) - 1) * m_verticalSpacing;
                    startY += std::max(0, (m_contentHeight - totalRowHeight) / 2);
                }
                int currentY = startY;
                for (qsizetype rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
                    auto* widget = column.widgets.at(rowIndex);
                    layoutCellWidget(widget, x, currentY, column.width, m_rowHeights.at(rowIndex));
                    currentY += m_rowHeights.at(rowIndex) + m_verticalSpacing;
                }
            }

            x += column.width;
            if (columnIndex + 1 < m_columns.size()) {
                x += m_horizontalSpacing;
            }
        }
    }

    const CommandTabModel::CommandTabTheme* m_theme = nullptr;
    QVector<PanelColumn> m_columns;
    QVector<QWidget*> m_pendingSmallColumn;
    QVector<QWidget*> m_pendingMediumColumn;
    QVector<int> m_rowHeights = {46, 46, 46};
    QSize m_preferredSize = QSize(120, 136);
    QMargins m_margins = QMargins(6, 4, 6, 4);
    int m_horizontalSpacing = 2;
    int m_verticalSpacing = 2;
    int m_contentHeight = 0;
    int m_mediumRowHeight = 0;
    bool m_iconOnlyPackingMode = false;
    CommandTabSettingsState m_settingsState;
};
