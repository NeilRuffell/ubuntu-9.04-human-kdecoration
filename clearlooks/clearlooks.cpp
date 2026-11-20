#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButton>
#include <KDecoration3/DecorationButtonGroup>
#include <KDecoration3/DecorationSettings>
#include <KPluginFactory>

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace Clearlooks
{
using KDecoration3::BorderSize;
using KDecoration3::DecoratedWindow;
using KDecoration3::DecorationButton;
using KDecoration3::DecorationButtonGroup;
using KDecoration3::DecorationButtonType;
using KDecoration3::DecorationSettings;

static const QColor ActiveBlue("#729fcf");
static const QColor InactiveGrey("#dedbd6");

static QLinearGradient gloss(const QRectF &rect, const QColor &base, bool pressed = false)
{
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    if (pressed) {
        gradient.setColorAt(0, base.darker(124));
        gradient.setColorAt(1, base);
    } else {
        gradient.setColorAt(0, base.lighter(112));
        gradient.setColorAt(0.49, base.lighter(103));
        gradient.setColorAt(0.50, base);
        gradient.setColorAt(1, base.darker(108));
    }
    return gradient;
}

// Keep bottom corners square, including when the side borders are visible.
static QPainterPath framePath(const QRectF &rect, qreal radius)
{
    QPainterPath path;
    path.moveTo(rect.left(), rect.bottom());
    path.lineTo(rect.left(), rect.top() + radius);
    path.quadTo(rect.topLeft(), rect.topLeft() + QPointF(radius, 0));
    path.lineTo(rect.topRight() - QPointF(radius, 0));
    path.quadTo(rect.topRight(), rect.topRight() + QPointF(0, radius));
    path.lineTo(rect.bottomRight());
    path.closeSubpath();
    return path;
}

static QPainterPath glyph(DecorationButtonType type, bool checked)
{
    // Glyphs are drawn around the button center in logical coordinates.
    QPainterPath path;
    const auto line = [&path](qreal x1, qreal y1, qreal x2, qreal y2) {
        path.moveTo(x1, y1);
        path.lineTo(x2, y2);
    };
    switch (type) {
    case DecorationButtonType::Close:
        line(-3, -3, 3, 3);
        line(3, -3, -3, 3);
        break;
    case DecorationButtonType::Minimize:
        line(-3.5, 2.5, 3.5, 2.5);
        break;
    case DecorationButtonType::Maximize:
        path.addRect(QRectF(-3.5, -3.5, 7, 7));
        line(-3.5, -1.5, 3.5, -1.5);
        if (checked)
            path.addRect(QRectF(-1.5, 0, 3, 1.5));
        break;
    case DecorationButtonType::Menu:
    case DecorationButtonType::ApplicationMenu:
        line(-3.5, -3, 3.5, -3);
        line(-3.5, 0, 3.5, 0);
        line(-3.5, 3, 3.5, 3);
        break;
    case DecorationButtonType::ContextHelp:
        path.moveTo(-2.5, -2);
        path.cubicTo(-2.5, -5, 3.5, -5, 2.5, -1.5);
        path.cubicTo(2, 0, 0, 0, 0, 1.5);
        line(0, 3.5, 0, 4);
        break;
    case DecorationButtonType::OnAllDesktops:
        path.addEllipse(QRectF(-3.5, -3.5, 7, 7));
        if (checked)
            path.addEllipse(QRectF(-1, -1, 2, 2));
        break;
    case DecorationButtonType::KeepAbove:
    case DecorationButtonType::KeepBelow:
    case DecorationButtonType::Shade: {
        const qreal direction = (type == DecorationButtonType::KeepBelow
                                || (type == DecorationButtonType::Shade && checked)) ? -1 : 1;
        path.moveTo(-3, 1.5 * direction);
        path.lineTo(0, -1.5 * direction);
        path.lineTo(3, 1.5 * direction);
        if (type == DecorationButtonType::Shade)
            line(-3, -4 * direction, 3, -4 * direction);
        break;
    }
    case DecorationButtonType::Custom:
    case DecorationButtonType::Spacer:
        break;
    }
    return path;
}

class Button : public DecorationButton
{
public:
    Button(DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
        : DecorationButton(type, decoration, parent)
    {
        connect(this, &Button::hoveredChanged, this, [this] { update(); });
        connect(this, &Button::pressedChanged, this, [this] { update(); });
        connect(this, &Button::checkedChanged, this, [this] { update(); });
        connect(this, &Button::enabledChanged, this, [this] { update(); });
    }

    void paint(QPainter *painter, const QRectF &repaintArea) override
    {
        if (type() == DecorationButtonType::Spacer || !repaintArea.intersects(geometry()))
            return;

        const bool active = decoration()->window()->isActive();
        const QColor base = active ? ActiveBlue : InactiveGrey;
        const QRectF face = geometry().adjusted(1.5, 2.5, -1.5, -2.5);
        const QColor fill = isHovered() && isEnabled() ? base.lighter(115) : base;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setOpacity(isEnabled() ? 1.0 : 0.55);
        painter->setPen(QPen(base.darker(149), 1));
        painter->setBrush(gloss(face, fill, isPressed()));
        painter->drawRoundedRect(face, 2.5, 2.5);

        painter->setPen(QPen(fill.lighter(isPressed() ? 104 : 136), 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(face.adjusted(1, 1, -1, -1), 1.5, 1.5);

        painter->translate(face.center() + QPointF(0, isPressed() ? 0.75 : 0));
        const QPainterPath symbol = glyph(type(), isChecked());
        painter->setPen(QPen(active ? base.darker(158) : base.lighter(119),
                              2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(symbol);
        painter->setPen(QPen(active ? QColor("#f5f9ff") : QColor("#57544f"),
                              1.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(symbol);
        painter->restore();
    }
};

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    Decoration(QObject *parent, const QVariantList &args)
        : KDecoration3::Decoration(parent, args)
    {
    }

    bool init() override
    {
        const auto createButton = [](DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
            -> DecorationButton * {
            if (type == DecorationButtonType::Custom)
                return nullptr;
            const qreal height = titleHeight(decoration->settings()->font());
            auto *button = new Button(type, decoration, parent);
            button->setGeometry(QRectF(0, 0, buttonWidth(type, height), height));
            return button;
        };

        m_left = new DecorationButtonGroup(DecorationButtonGroup::Position::Left, this, createButton);
        m_right = new DecorationButtonGroup(DecorationButtonGroup::Position::Right, this, createButton);

        // Group relayout can be underway when its geometry signal arrives.
        connect(m_right, &DecorationButtonGroup::geometryChanged, this, &Decoration::positionButtons,
                Qt::QueuedConnection);
        connect(m_left, &DecorationButtonGroup::geometryChanged, this, [this] { update(titleBar()); });
        connect(this, &Decoration::bordersChanged, this, &Decoration::positionButtons);
        connect(window(), &DecoratedWindow::widthChanged, this, &Decoration::positionButtons);
        connect(window(), &DecoratedWindow::maximizedHorizontallyChanged, this, &Decoration::layout);
        connect(window(), &DecoratedWindow::maximizedVerticallyChanged, this, &Decoration::layout);
        connect(window(), &DecoratedWindow::activeChanged, this, [this] { update(); });
        connect(window(), &DecoratedWindow::captionChanged, this, [this] { update(titleBar()); });
        connect(settings().get(), &DecorationSettings::fontChanged, this, &Decoration::layout);
        connect(settings().get(), &DecorationSettings::borderSizeChanged, this, &Decoration::layout);
        connect(settings().get(), &DecorationSettings::alphaChannelSupportedChanged, this, [this] { update(); });
        layout();
        return true;
    }

    void paint(QPainter *painter, const QRectF &repaintArea) override
    {
        const QRectF outer = rect();
        if (outer.isEmpty())
            return;

        const bool active = window()->isActive();
        const QColor base = active ? ActiveBlue : InactiveGrey.darker(105);
        const qreal radius = window()->isMaximizedHorizontally() || !settings()->isAlphaChannelSupported()
            ? 0 : std::min(5.0, outer.width() / 2);
        const QRectF title = titleBar();

        painter->save();
        painter->setClipRect(repaintArea, Qt::IntersectClip);
        painter->setRenderHint(QPainter::Antialiasing);

        QPainterPath border = framePath(outer, radius);
        QPainterPath clientArea;
        clientArea.addRect(outer.marginsRemoved(borders()));
        border = border.subtracted(clientArea);
        painter->fillPath(border, InactiveGrey.darker(127));
        painter->fillPath(framePath(title, radius), gloss(title, base));

        painter->setPen(QPen(base.darker(154), 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(framePath(title.adjusted(0.5, 0.5, -0.5, -0.5), std::max(0.0, radius - 0.5)));
        QPainterPath highlight;
        highlight.moveTo(1.5, title.height() - 1.5);
        highlight.lineTo(1.5, 1.5 + radius);
        highlight.quadTo(1.5, 1.5, 1.5 + radius, 1.5);
        highlight.lineTo(title.right() - 1.5 - radius, 1.5);
        painter->setPen(QPen(base.lighter(133), 1));
        painter->drawPath(highlight);

        paintCaption(painter, title, active);
        m_left->paint(painter, repaintArea);
        m_right->paint(painter, repaintArea);
        painter->restore();
    }

private:
    static qreal titleHeight(const QFont &font)
    {
        return std::max(23.0, QFontMetricsF(font).height() + 7);
    }

    static qreal buttonWidth(DecorationButtonType type, qreal height)
    {
        return type == DecorationButtonType::Spacer ? 9 : height - 3;
    }

    void layout()
    {
        constexpr qreal widths[] = {0, 4, 2, 4, 6, 8, 10, 12, 16};
        const BorderSize borderSize = settings()->borderSize();
        const qreal frame = widths[std::clamp(static_cast<int>(borderSize), 0, 8)];
        const bool horizontal = window()->isMaximizedHorizontally();
        const bool vertical = window()->isMaximizedVertically();
        const qreal side = horizontal || borderSize == BorderSize::NoSides ? 0 : frame;
        const qreal bottom = vertical ? std::min(frame, 1.0) : frame;
        const qreal height = titleHeight(settings()->font());

        setBorders(QMarginsF(side, height, side, bottom));
        setResizeOnlyBorders(QMarginsF(horizontal ? 0 : 5, 0, horizontal ? 0 : 5, vertical ? 0 : 5));

        for (auto *button : m_left->buttons() + m_right->buttons()) {
            button->setGeometry(QRectF(button->geometry().topLeft(),
                                       QSizeF(buttonWidth(button->type(), height), height)));
        }

        positionButtons();
        update();
    }

    void positionButtons()
    {
        const qreal edge = window()->isMaximizedHorizontally() ? 1 : 4;
        setTitleBar(QRectF(0, 0, size().width(), borderTop()));
        m_left->setPos(QPointF(edge, 0));
        m_right->setPos(QPointF(std::max(edge, size().width() - edge - m_right->geometry().width()), 0));
        update(titleBar());
    }

    void paintCaption(QPainter *painter, const QRectF &title, bool active)
    {
        const qreal left = (m_left->buttons().isEmpty() ? m_left->pos().x() : m_left->geometry().right()) + 5;
        const qreal right = (m_right->buttons().isEmpty() ? title.right() - m_left->pos().x()
                                                         : m_right->geometry().left()) - 5;
        if (right <= left)
            return;

        painter->setFont(settings()->font());
        const QFontMetricsF metrics(painter->font());
        const QString text = metrics.elidedText(window()->caption(), Qt::ElideRight, right - left);
        const qreal width = metrics.horizontalAdvance(text);
        const qreal x = std::clamp(title.center().x() - width / 2, left, std::max(left, right - width));
        const QPointF baseline(x, title.center().y() + (metrics.ascent() - metrics.descent()) / 2);
        if (active) {
            painter->setPen(QColor("#35577d"));
            painter->drawText(baseline + QPointF(0, 1), text);
        }
        painter->setPen(active ? Qt::white : QColor("#4d4a46"));
        painter->drawText(baseline, text);
    }

    DecorationButtonGroup *m_left = nullptr;
    DecorationButtonGroup *m_right = nullptr;
};
}

K_PLUGIN_FACTORY_WITH_JSON(ClearlooksFactory, "clearlooks.json",
    registerPlugin<Clearlooks::Decoration>();)

#include "clearlooks.moc"
