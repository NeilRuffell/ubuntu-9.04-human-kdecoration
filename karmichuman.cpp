#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButton>
#include <KDecoration3/DecorationButtonGroup>
#include <KDecoration3/DecorationSettings>

#include <KPluginFactory>
#include <KColorScheme>
#include <KColorUtils>

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRegion>

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace KarmicHuman
{

using KDecoration3::DecoratedWindow;
using KDecoration3::DecorationButtonType;

static constexpr qreal TitleHeight = 24.0;
static constexpr qreal FrameWidth = 5.0;
static constexpr qreal TitleEdge = 4.0;
static constexpr qreal ButtonHeight = 24.0;
static constexpr qreal ButtonWidth = 26.0;

// Original Ubuntu 9.04 Human GTK theme palette (gtk-2.0/gtkrc):
// selected_bg_color=#8F5F4A, selected_fg_color=#FFFFFF,
// bg_color=#E6DDD5, fg_color=#101010.
// KDE's inactive Window background is retained per this port's requirement
// to blend with the application, without changing the Metacity gradients.
static const QColor HumanNormalBg("#E6DDD5");
static const QColor HumanSelectedBg("#8F5F4A");
static const QColor HumanNormalFg("#101010");
static const QColor HumanSelectedFg("#FFFFFF");

static QColor inactiveHumanBackground(const DecoratedWindow *window)
{
    const QColor color = window->palette().color(QPalette::Inactive, QPalette::Window);
    return color.isValid() ? color : HumanNormalBg;
}

// Oxygen liboxygen/oxygenhelper.cpp (Plasma/6.8): calcLightColor and
// highThreshold, without Oxygen's colour cache. Used for contrast passes.
static QColor oxygenLightColor(const QColor &base)
{
    const QColor lighter = KColorScheme::shade(base, KColorScheme::LightShade, 0.5);
    const bool highThreshold = KColorUtils::luma(lighter) < KColorUtils::luma(base);
    return highThreshold ? base
                         : KColorScheme::shade(base, KColorScheme::LightShade,
                                               KColorScheme::contrastF());
}

static void rgbToHls(double &r, double &g, double &b)
{
    const double minv = std::min({r, g, b});
    const double maxv = std::max({r, g, b});

    double h = 0.0;
    double s = 0.0;
    const double l = (maxv + minv) / 2.0;

    if (maxv != minv) {
        const double delta = maxv - minv;

        if (l <= 0.5)
            s = delta / (maxv + minv);
        else
            s = delta / (2.0 - maxv - minv);

        if (r == maxv)
            h = (g - b) / delta;
        else if (g == maxv)
            h = 2.0 + (b - r) / delta;
        else
            h = 4.0 + (r - g) / delta;

        h *= 60.0;

        if (h < 0.0)
            h += 360.0;
    }

    r = h;
    g = l;
    b = s;
}

static double hlsValue(double n1, double n2, double hue)
{
    if (hue > 360.0)
        hue -= 360.0;
    else if (hue < 0.0)
        hue += 360.0;

    if (hue < 60.0)
        return n1 + (n2 - n1) * hue / 60.0;
    if (hue < 180.0)
        return n2;
    if (hue < 240.0)
        return n1 + (n2 - n1) * (240.0 - hue) / 60.0;

    return n1;
}

static void hlsToRgb(double &h, double &l, double &s)
{
    double r;
    double g;
    double b;

    if (s == 0.0) {
        r = g = b = l;
    } else {
        const double m2 =
            (l <= 0.5)
                ? l * (1.0 + s)
                : l + s - l * s;

        const double m1 = 2.0 * l - m2;

        r = hlsValue(m1, m2, h + 120.0);
        g = hlsValue(m1, m2, h);
        b = hlsValue(m1, m2, h - 120.0);
    }

    h = r;
    l = g;
    s = b;
}

static QColor shade(const QColor &input, double factor)
{
    double h = input.redF();
    double l = input.greenF();
    double s = input.blueF();

    rgbToHls(h, l, s);

    l = std::clamp(l * factor, 0.0, 1.0);
    s = std::clamp(s * factor, 0.0, 1.0);

    hlsToRgb(h, l, s);

    return QColor::fromRgbF(
        std::clamp(h, 0.0, 1.0),
        std::clamp(l, 0.0, 1.0),
        std::clamp(s, 0.0, 1.0),
        input.alphaF());
}

// Oxygen's KColorUtils contrast technique, applied exclusively to Human
// foreground colours. Never substitute Oxygen/KDE accent colours for Human.
// Use original Human dark text where readable; on a dark inactive background
// switch to Human's original white selected foreground.
static QColor inactiveHumanForeground(const QColor &background,
                                      std::initializer_list<double> shadeFactors,
                                      qreal requiredContrast)
{
    const auto minimumContrast = [&](const QColor &foreground) {
        qreal ratio = 100.0;
        for (double factor : shadeFactors) {
            ratio = std::min(ratio,
                             KColorUtils::contrastRatio(foreground,
                                                        shade(background, factor)));
        }
        return ratio;
    };

    const qreal normalContrast = minimumContrast(HumanNormalFg);
    if (normalContrast >= requiredContrast
        || normalContrast >= minimumContrast(HumanSelectedFg))
        return HumanNormalFg;
    return HumanSelectedFg;
}

static QColor blend(const QColor &bg,
                    const QColor &fg,
                    double alpha)
{
    const auto component = [alpha](double b, double f) {
        return std::clamp(b + (f - b) * alpha, 0.0, 1.0);
    };

    return QColor::fromRgbF(
        component(bg.redF(), fg.redF()),
        component(bg.greenF(), fg.greenF()),
        component(bg.blueF(), fg.blueF()),
        component(bg.alphaF(), fg.alphaF()));
}

static QColor withAlpha(QColor c, double alpha)
{
    c.setAlphaF(alpha);
    return c;
}

static void pixelLine(QPainter *p,
                      const QColor &c,
                      int x1, int y1,
                      int x2, int y2)
{
    p->setPen(QPen(c, 1));
    p->drawLine(x1, y1, x2, y2);
}

/*
 * Metacity's rounded Human frame is a pixel-stepped mask, not a
 * continuously antialiased Qt rounded rectangle.
 *
 * The top-left silhouette is:
 *   y=0: starts at x=5
 *   y=1: starts at x=3
 *   y=2: starts at x=2
 *   y=3..4: starts at x=1
 *   y>=5: full width
 *
 * The other corners mirror that shape.
 */
static QRegion humanWindowRegion(int w, int h)
{
    if (w <= 10 || h <= 10)
        return QRegion(0, 0, w, h);

    QRegion region;

    region += QRect(5, 0, w - 10, 1);
    region += QRect(3, 1, w - 6, 1);
    region += QRect(2, 2, w - 4, 1);
    region += QRect(1, 3, w - 2, 2);
    region += QRect(0, 5, w, h - 10);
    region += QRect(1, h - 5, w - 2, 2);
    region += QRect(2, h - 3, w - 4, 1);
    region += QRect(3, h - 2, w - 6, 1);
    region += QRect(5, h - 1, w - 10, 1);

    return region;
}

static void paintTopCornerDetails(QPainter *p, int w, bool active,
                                  const QColor &NormalBg, const QColor &SelectedBg)
{
    if (active) {
        /* corners_outline_selected_top */
        pixelLine(p, shade(SelectedBg, 0.40), 0, 6, 1, 2);
        pixelLine(p, shade(SelectedBg, 0.50), 1, 3, 1, 4);
        pixelLine(p, shade(SelectedBg, 0.55), 1, 3, 2, 2);
        pixelLine(p, shade(SelectedBg, 0.55), 3, 1, 4, 1);
        pixelLine(p, shade(SelectedBg, 0.60), 3, 1, 1, 1);

        pixelLine(p, shade(SelectedBg, 0.40), w - 1, 6, w - 1, 2);
        pixelLine(p, shade(SelectedBg, 0.50), w - 2, 3, w - 2, 4);
        pixelLine(p, shade(SelectedBg, 0.55), w - 2, 3, w - 3, 2);
        pixelLine(p, shade(SelectedBg, 0.55), w - 4, 1, w - 5, 1);
        pixelLine(p, shade(SelectedBg, 0.60), w - 4, 1, w - 1, 1);
    } else {
        /* corners_outline_top */
        pixelLine(p, shade(NormalBg, 0.35), 1, 3, 1, 4);
        pixelLine(p, shade(NormalBg, 0.40), 2, 2, 2, 2);
        pixelLine(p, shade(NormalBg, 0.45), 3, 1, 4, 1);

        pixelLine(p, shade(NormalBg, 0.35), w - 2, 3, w - 2, 4);
        pixelLine(p, shade(NormalBg, 0.40), w - 3, 2, w - 3, 2);
        pixelLine(p, shade(NormalBg, 0.45), w - 4, 1, w - 5, 1);
    }

    /* corners_hilight -- used by both focused and unfocused round bevels */
    const QColor selectedHi = shade(SelectedBg, 2.0);

    p->fillRect(QRectF(2, 3, 1, 2), withAlpha(selectedHi, 0.30));
    p->fillRect(QRectF(3, 3, 1, 1), withAlpha(selectedHi, 0.20));
    p->fillRect(QRectF(3, 2, 2, 1), withAlpha(selectedHi, 0.30));
    p->fillRect(QRectF(3, 2, 1, 1), withAlpha(Qt::black, 0.07));
    p->fillRect(QRectF(2, 5, 1, 1), withAlpha(selectedHi, 0.20));
    p->fillRect(QRectF(2, 6, 1, 1), withAlpha(selectedHi, 0.05));
    p->fillRect(QRectF(4, 3, 1, 1), withAlpha(selectedHi, 0.10));
    p->fillRect(QRectF(3, 4, 1, 1), withAlpha(selectedHi, 0.10));
    p->fillRect(QRectF(5, 2, 1, 1), withAlpha(selectedHi, 0.20));

    p->fillRect(QRectF(w - 4, 3, 1, 1), withAlpha(selectedHi, 0.15));
    p->fillRect(QRectF(w - 4, 2, 1, 1), withAlpha(Qt::black, 0.05));
    p->fillRect(QRectF(w - 5, 2, 1, 1), withAlpha(selectedHi, 0.20));
    p->fillRect(QRectF(w - 6, 2, 1, 1), withAlpha(selectedHi, 0.10));
    p->fillRect(QRectF(w - 7, 2, 1, 1), withAlpha(selectedHi, 0.05));
    p->fillRect(QRectF(w - 3, 3, 1, 1), withAlpha(Qt::black, 0.10));
    p->fillRect(QRectF(w - 3, 4, 1, 2), withAlpha(Qt::black, 0.04));
}

static void paintBottomCornerDetails(QPainter *p, int w, int h, const QColor &NormalBg)
{
    /* corners_outline_bottom */
    pixelLine(p, shade(NormalBg, 0.95), 4, h - 3, 1, h - 3);
    pixelLine(p, shade(NormalBg, 0.93), 3, h - 3, 1, h - 3);
    pixelLine(p, shade(NormalBg, 1.05), 2, h - 5, 1, h - 6);
    pixelLine(p, shade(NormalBg, 1.03), 2, h - 4, 1, h - 5);
    pixelLine(p, shade(NormalBg, 0.85), 6, h - 2, 1, h - 2);
    pixelLine(p, shade(NormalBg, 0.83), 5, h - 2, 1, h - 3);

    pixelLine(p, shade(NormalBg, 0.28), 1, h - 4, 1, h - 5);
    pixelLine(p, shade(NormalBg, 0.30), 2, h - 3, 2, h - 3);
    pixelLine(p, shade(NormalBg, 0.28), 2, h - 2, 4, h - 2);

    pixelLine(p, shade(NormalBg, 0.90), w - 3, h - 4, w - 2, h - 4);
    pixelLine(p, shade(NormalBg, 0.90), w - 3, h - 5, w - 2, h - 5);
    pixelLine(p, shade(NormalBg, 0.90), w - 4, h - 3, w - 5, h - 3);
    pixelLine(p, shade(NormalBg, 0.97), w - 4, h - 4, w - 4, h - 4);

    pixelLine(p, shade(NormalBg, 0.28), w - 2, h - 4, w - 2, h - 5);
    pixelLine(p, shade(NormalBg, 0.30), w - 3, h - 3, w - 3, h - 3);
    pixelLine(p, shade(NormalBg, 0.28), w - 4, h - 2, w - 5, h - 2);
}

class Decoration;

class Button : public KDecoration3::DecorationButton
{
    Q_OBJECT

public:
    Button(DecorationButtonType type,
           Decoration *decoration,
           QObject *parent = nullptr);

    Button(QObject *parent, const QVariantList &args);

    static Button *create(
        DecorationButtonType type,
        KDecoration3::Decoration *decoration,
        QObject *parent);

    void paint(QPainter *painter,
               const QRectF &repaintArea) override;
};

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    explicit Decoration(QObject *parent = nullptr,
                        const QVariantList &args = QVariantList())
        : KDecoration3::Decoration(parent, args)
    {
    }

    bool init() override;

    void paint(QPainter *painter,
               const QRectF &repaintArea) override;

    void updateLayout();

private:
    void createButtons();
    void paintFrame(QPainter *painter);
    void paintTitleBar(QPainter *painter);
    QRectF captionRect() const;

    KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;
};

Button::Button(DecorationButtonType type,
               Decoration *decoration,
               QObject *parent)
    : KDecoration3::DecorationButton(type, decoration, parent)
{
    if (type == DecorationButtonType::Spacer)
        setGeometry(QRectF(0, 0, 9, ButtonHeight));
    else
        setGeometry(QRectF(0, 0, ButtonWidth, ButtonHeight));

    connect(this,
            &KDecoration3::DecorationButton::hoveredChanged,
            this,
            [this]() { update(); });

    connect(this,
            &KDecoration3::DecorationButton::pressedChanged,
            this,
            [this]() { update(); });

    connect(decoration->window(),
            &DecoratedWindow::activeChanged,
            this,
            [this]() { update(); });

    connect(decoration->window(),
            &DecoratedWindow::maximizedChanged,
            this,
            [this]() { update(); });

    connect(decoration->window(),
            &DecoratedWindow::iconChanged,
            this,
            [this]() { update(); });
}

Button::Button(QObject *parent, const QVariantList &args)
    : Button(args.at(0).value<DecorationButtonType>(),
             args.at(1).value<Decoration *>(),
             parent)
{
}

Button *Button::create(DecorationButtonType type,
                       KDecoration3::Decoration *decoration,
                       QObject *parent)
{
    auto *d = qobject_cast<Decoration *>(decoration);

    if (!d)
        return nullptr;

    switch (type) {
    case DecorationButtonType::Menu:
    case DecorationButtonType::Minimize:
    case DecorationButtonType::Maximize:
    case DecorationButtonType::Close:
    case DecorationButtonType::Spacer:
        break;

    default:
        return nullptr;
    }

    auto *button = new Button(type, d, parent);
    auto *w = d->window();

    switch (type) {
    case DecorationButtonType::Close:
        button->setVisible(w->isCloseable());
        connect(w,
                &DecoratedWindow::closeableChanged,
                button,
                &Button::setVisible);
        break;

    case DecorationButtonType::Maximize:
        button->setVisible(w->isMaximizeable());
        connect(w,
                &DecoratedWindow::maximizeableChanged,
                button,
                &Button::setVisible);
        break;

    case DecorationButtonType::Minimize:
        button->setVisible(w->isMinimizeable());
        connect(w,
                &DecoratedWindow::minimizeableChanged,
                button,
                &Button::setVisible);
        break;

    default:
        break;
    }

    return button;
}

static void paintButtonOutline(QPainter *p,
                               const QRectF &r)
{
    const int x = qRound(r.x());
    const int y = qRound(r.y());
    const int w = qRound(r.width());
    const int h = qRound(r.height());

    const QColor black45 = withAlpha(Qt::black, 0.45);
    const QColor white40 = withAlpha(Qt::white, 0.40);
    const QColor black20 = withAlpha(Qt::black, 0.20);

    p->fillRect(QRectF(x + 2, y + 3, w - 4, 1), black45);
    p->fillRect(QRectF(x + 2, y + h - 4, w - 4, 1), black45);
    p->fillRect(QRectF(x + 1, y + 4, 1, h - 8), black45);
    p->fillRect(QRectF(x + w - 2, y + 4, 1, h - 8), black45);

    p->fillRect(QRectF(x + 2, y + 4, w - 4, 1), white40);
    p->fillRect(QRectF(x + 2, y + h - 5, w - 4, 1), white40);
    p->fillRect(QRectF(x + 2, y + 5, 1, h - 10), white40);
    p->fillRect(QRectF(x + w - 3, y + 5, 1, h - 10), white40);

    p->fillRect(QRectF(x + 2, y + 4, 1, 1), black20);
    p->fillRect(QRectF(x + 2, y + h - 5, 1, 1), black20);
    p->fillRect(QRectF(x + w - 3, y + 4, 1, 1), black20);
    p->fillRect(QRectF(x + w - 3, y + h - 5, 1, 1), black20);
}

static void paintButtonBackground(QPainter *p,
                                  const QRectF &r,
                                  bool active,
                                  bool hovered,
                                  bool pressed,
                                  const QColor &base)
{
    double a;
    double b;
    double c;
    double d;

    if (active) {
        if (pressed) {
            a = 1.15;
            b = 0.75;
            c = 0.70;
            d = 0.75;
        } else if (hovered) {
            a = 1.45;
            b = 1.05;
            c = 0.95;
            d = 1.05;
        } else {
            a = 1.35;
            b = 0.95;
            c = 0.90;
            d = 0.95;
        }
    } else {
        if (pressed) {
            a = 0.76;
            b = 0.75;
            c = 0.73;
            d = 0.75;
        } else if (hovered) {
            a = 1.30;
            b = 1.10;
            c = 1.05;
            d = 1.10;
        } else {
            a = 1.10;
            b = 1.00;
            c = 0.98;
            d = 1.00;
        }
    }

    const qreal half = r.height() / 2.0;

    const QRectF top(
        r.x() + 2,
        r.y() + 4,
        r.width() - 4,
        half - 4);

    const QRectF bottom(
        r.x() + 2,
        r.y() + half,
        r.width() - 4,
        half - 3);

    QLinearGradient gt(top.topLeft(), top.bottomLeft());
    gt.setColorAt(0.0, shade(base, a));
    gt.setColorAt(1.0, shade(base, b));
    p->fillRect(top, gt);

    QLinearGradient gb(bottom.topLeft(), bottom.bottomLeft());
    gb.setColorAt(0.0, shade(base, c));
    gb.setColorAt(1.0, shade(base, d));
    p->fillRect(bottom, gb);

    paintButtonOutline(p, r);
}

void Button::paint(QPainter *painter,
                   const QRectF &repaintArea)
{
    Q_UNUSED(repaintArea)

    auto *d = qobject_cast<Decoration *>(decoration());

    if (!d)
        return;

    if (type() == DecorationButtonType::Spacer)
        return;

    const QRectF r = geometry();
    const bool active = d->window()->isActive();

    if (type() == DecorationButtonType::Menu) {
        const QIcon icon = d->window()->icon();

        if (!icon.isNull()) {
            const QSize iconSize(16, 16);
            const QPixmap pm = icon.pixmap(iconSize);

            const qreal x =
                r.x() + (r.width() - iconSize.width()) / 2.0 - 2.0;

            const qreal y =
                r.y() + (r.height() - iconSize.height()) / 2.0 + 1.0;

            painter->drawPixmap(
                QRectF(x, y,
                       iconSize.width(),
                       iconSize.height()),
                pm,
                QRectF(pm.rect()));
        }

        return;
    }

    const QColor base = active ? HumanSelectedBg : inactiveHumanBackground(d->window());
    paintButtonBackground(
        painter,
        r,
        active,
        isHovered(),
        isPressed(),
        base);

    QString resource;

    switch (type()) {
    case DecorationButtonType::Close:
        resource = active
            ? QStringLiteral(":/human/icon_close.png")
            : QStringLiteral(":/human/icon_close_u.png");
        break;

    case DecorationButtonType::Minimize:
        resource = active
            ? QStringLiteral(":/human/icon_minimize.png")
            : QStringLiteral(":/human/icon_minimize_u.png");
        break;

    case DecorationButtonType::Maximize:
        if (isChecked()) {
            resource = active
                ? QStringLiteral(":/human/icon_restore.png")
                : QStringLiteral(":/human/icon_restore_u.png");
        } else {
            resource = active
                ? QStringLiteral(":/human/icon_maximize.png")
                : QStringLiteral(":/human/icon_maximize_u.png");
        }
        break;

    default:
        return;
    }

    QImage image(resource);

    if (!image.isNull()) {
        // Retain the exact original silhouette/alpha, but recolour the glyph
        // with the scheme foreground colour instead of its static PNG RGB.
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QPainter tint(&image);
        tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
        // Preserve the original Human white active glyphs. Only inactive
        // glyphs change between the original #101010 and #FFFFFF, based on
        // contrast against Human's actual inactive button gradient.
        const QColor glyphColor = active
            ? HumanSelectedFg
            : (isPressed()
                ? inactiveHumanForeground(base, {0.76, 0.75, 0.73, 0.75}, 3.0)
                : isHovered()
                    ? inactiveHumanForeground(base, {1.30, 1.10, 1.05, 1.10}, 3.0)
                    : inactiveHumanForeground(base, {1.10, 1.00, 0.98, 1.00}, 3.0));
        tint.fillRect(image.rect(), glyphColor);
        tint.end();
        const QRectF target(
            r.x() + (r.width() - 10.0) / 2.0,
            r.y() + (r.height() - 10.0) / 2.0,
            10.0,
            10.0);

        // Oxygen draws a light, offset contrast pass before its icon.
        // Reuse the original Human alpha mask for both active and inactive.
        QImage contrastImage(resource);
        if (!contrastImage.isNull()) {
            contrastImage = contrastImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            QPainter contrastPainter(&contrastImage);
            contrastPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            contrastPainter.fillRect(contrastImage.rect(),
                oxygenLightColor(base));
            contrastPainter.end();
            painter->drawImage(target.translated(0, 1.0), contrastImage);
        }
        painter->drawImage(target, image);
    }
}

bool Decoration::init()
{
    createButtons();

    connect(window(),
            &DecoratedWindow::widthChanged,
            this,
            &Decoration::updateLayout);

    connect(window(),
            &DecoratedWindow::maximizedChanged,
            this,
            &Decoration::updateLayout);

    connect(window(),
            &DecoratedWindow::activeChanged,
            this,
            [this]() { update(); });

    connect(window(),
            &DecoratedWindow::captionChanged,
            this,
            [this]() { update(titleBar()); });

    connect(window(),
            &DecoratedWindow::iconChanged,
            this,
            [this]() { update(titleBar()); });

    connect(window(),
            &DecoratedWindow::paletteChanged,
            this,
            [this]() {
                update();
                // Button glyphs are tinted using the same palette. Explicitly
                // invalidate them as well as the decoration background.
                for (auto *button : m_leftButtons->buttons())
                    button->update();
                for (auto *button : m_rightButtons->buttons())
                    button->update();
            });

    connect(settings().get(),
            &KDecoration3::DecorationSettings::decorationButtonsLeftChanged,
            this,
            &Decoration::updateLayout);

    connect(settings().get(),
            &KDecoration3::DecorationSettings::decorationButtonsRightChanged,
            this,
            &Decoration::updateLayout);

    updateLayout();

    return true;
}

void Decoration::createButtons()
{
    m_leftButtons =
        new KDecoration3::DecorationButtonGroup(
            KDecoration3::DecorationButtonGroup::Position::Left,
            this,
            &Button::create);

    m_rightButtons =
        new KDecoration3::DecorationButtonGroup(
            KDecoration3::DecorationButtonGroup::Position::Right,
            this,
            &Button::create);

    m_leftButtons->setSpacing(0);
    m_rightButtons->setSpacing(0);
}

void Decoration::updateLayout()
{
    const bool maximized = window()->isMaximized();

    setBorders(QMarginsF(
        maximized ? 0 : FrameWidth,
        TitleHeight,
        maximized ? 0 : FrameWidth,
        maximized ? 1 : FrameWidth));

    setResizeOnlyBorders(
        maximized
            ? QMarginsF()
            : QMarginsF(5, 0, 5, 5));

    setBorderRadius(
        maximized
            ? KDecoration3::BorderRadius()
            : KDecoration3::BorderRadius(6.0));

    setTitleBar(QRectF(
        0,
        0,
        size().width(),
        TitleHeight));

    for (auto *button :
         m_leftButtons->buttons() + m_rightButtons->buttons()) {

        if (button->type() == DecorationButtonType::Spacer) {
            button->setGeometry(
                QRectF(0, 0, 9, ButtonHeight));
        } else {
            button->setGeometry(
                QRectF(0, 0, ButtonWidth, ButtonHeight));
        }
    }

    const qreal edge = maximized ? 0.0 : TitleEdge;

    m_leftButtons->setPos(
        QPointF(edge, 0));

    m_rightButtons->setPos(
        QPointF(
            size().width()
                - edge
                - m_rightButtons->geometry().width(),
            0));

    update();
}

QRectF Decoration::captionRect() const
{
    const bool maximized = window()->isMaximized();
    const qreal edge = maximized ? 0.0 : TitleEdge;

    qreal left = edge + 2.0;
    qreal right = size().width() - edge - 2.0;

    if (m_leftButtons &&
        !m_leftButtons->buttons().isEmpty()) {

        left = qMax(
            left,
            m_leftButtons->geometry().right() + 2.0);
    }

    if (m_rightButtons &&
        !m_rightButtons->buttons().isEmpty()) {

        right = qMin(
            right,
            m_rightButtons->geometry().left() - 2.0);
    }

    return QRectF(
        left,
        0,
        qMax<qreal>(0, right - left),
        TitleHeight);
}

void Decoration::paintFrame(QPainter *p)
{
    // Human's normal beige frame surrounds focused brown titlebars.
    // Inactive frames follow the KDE window background as requested.
    const QColor NormalBg = window()->isActive()
        ? HumanNormalBg : inactiveHumanBackground(window());
    const QRectF r = rect();
    const int w = qRound(r.width());
    const int h = qRound(r.height());

    p->fillRect(r, NormalBg);

    if (window()->isMaximized())
        return;

    pixelLine(
        p,
        shade(NormalBg, 0.88),
        1, h - 2,
        w - 2, h - 2);

    pixelLine(
        p,
        shade(NormalBg, 0.88),
        w - 2, 3,
        w - 2, h - 2);

    pixelLine(
        p,
        shade(NormalBg, 1.30),
        3, 1,
        w - 4, 1);

    pixelLine(
        p,
        shade(NormalBg, 1.30),
        1, 3,
        1, h - 2);

    p->setPen(QPen(shade(NormalBg, 0.25), 1));
    p->setBrush(Qt::NoBrush);
    p->drawRect(QRectF(0, 0, w - 1, h - 1));

    paintBottomCornerDetails(p, w, h, NormalBg);
}

void Decoration::paintTitleBar(QPainter *p)
{
    const QColor NormalBg = inactiveHumanBackground(window());
    const QColor SelectedBg = HumanSelectedBg;
    const QColor NormalFg = inactiveHumanForeground(
        NormalBg, {1.05, 1.00, 0.99, 1.00}, 4.5);
    // The original Human active caption is white, with a dark brown
    // four-pass Metacity shadow; do not replace it with scheme foreground.
    const QColor ActiveFg = HumanSelectedFg;
    const int w = qRound(size().width());
    const int h = qRound(TitleHeight);
    const int half = h / 2;

    const bool active = window()->isActive();
    const bool maximized = window()->isMaximized();

    if (active) {
        QRectF top(0, 1, w, half);

        QLinearGradient gt(
            top.topLeft(),
            top.bottomLeft());

        gt.setColorAt(
            0.0,
            shade(SelectedBg, 1.30));

        gt.setColorAt(
            1.0,
            shade(SelectedBg, 1.00));

        p->fillRect(top, gt);

        QRectF bottom(
            0,
            half,
            w,
            half);

        QLinearGradient gb(
            bottom.topLeft(),
            bottom.bottomLeft());

        gb.setColorAt(
            0.0,
            shade(SelectedBg, 0.97));

        gb.setColorAt(
            1.0,
            shade(SelectedBg, 1.10));

        p->fillRect(bottom, gb);

        pixelLine(
            p,
            shade(SelectedBg, 0.55),
            0, 0,
            w - 1, 0);

        p->fillRect(
            QRectF(0, 1, w, 1),
            withAlpha(
                shade(SelectedBg, 2.0),
                0.30));

        if (w > 14) {
            p->fillRect(
                QRectF(7, 1, w - 14, 1),
                withAlpha(
                    shade(SelectedBg, 2.0),
                    0.05));
        }

        pixelLine(
            p,
            shade(SelectedBg, 0.97),
            0, h - 2,
            w - 1, h - 2);

        pixelLine(
            p,
            shade(SelectedBg, 0.70),
            0, h - 1,
            w - 1, h - 1);

        p->fillRect(
            QRectF(6, 2, 1, 1),
            withAlpha(
                shade(SelectedBg, 2.0),
                0.10));

        p->fillRect(
            QRectF(w - 6, 1, 1, 1),
            withAlpha(Qt::black, 0.05));

        p->fillRect(
            QRectF(5, 1, 1, 1),
            withAlpha(Qt::black, 0.05));

        if (!maximized) {
            pixelLine(
                p,
                shade(SelectedBg, 0.32),
                0, 0,
                0, h - 1);

            pixelLine(
                p,
                shade(SelectedBg, 0.32),
                w - 1, 0,
                w - 1, h - 1);

            p->fillRect(
                QRectF(1, 2, 1, h - 3),
                withAlpha(
                    shade(SelectedBg, 2.0),
                    0.30));

            p->fillRect(
                QRectF(w - 2, 2, 1, h - 4),
                withAlpha(Qt::black, 0.05));
        }

    } else {
        QRectF top(
            2,
            1,
            qMax(0, w - 4),
            half);

        QLinearGradient gt(
            top.topLeft(),
            top.bottomLeft());

        gt.setColorAt(
            0.0,
            shade(NormalBg, 1.05));

        gt.setColorAt(
            1.0,
            shade(NormalBg, 1.00));

        p->fillRect(top, gt);

        QRectF bottom(
            2,
            half,
            qMax(0, w - 4),
            half);

        QLinearGradient gb(
            bottom.topLeft(),
            bottom.bottomLeft());

        gb.setColorAt(
            0.0,
            shade(NormalBg, 0.99));

        gb.setColorAt(
            1.0,
            shade(NormalBg, 1.00));

        p->fillRect(bottom, gb);

        pixelLine(
            p,
            shade(NormalBg, 0.45),
            0, 0,
            w - 1, 0);

        pixelLine(
            p,
            shade(NormalBg, 1.30),
            0, 1,
            w - 1, 1);
    }

    if (!maximized)
        paintTopCornerDetails(p, w, active, NormalBg, SelectedBg);

    if (maximized) {
        pixelLine(
            p,
            shade(NormalBg, 0.70),
            0, h - 1,
            w - 1, h - 1);
    }

    QFont font(QStringLiteral("Sans"));
    font.setPointSize(10);
    font.setBold(true);

    p->setFont(font);

    QRectF textRect = captionRect();

    const QString caption =
        p->fontMetrics().elidedText(
            window()->caption(),
            Qt::ElideMiddle,
            static_cast<int>(textRect.width()));

    if (active) {
        p->setPen(shade(SelectedBg, 0.75));
        p->drawText(
            textRect.translated(1, 2),
            Qt::AlignCenter | Qt::TextSingleLine,
            caption);

        p->setPen(shade(SelectedBg, 0.70));
        p->drawText(
            textRect.translated(2, 2),
            Qt::AlignCenter | Qt::TextSingleLine,
            caption);

        p->setPen(shade(SelectedBg, 0.40));
        p->drawText(
            textRect.translated(1, 1),
            Qt::AlignCenter | Qt::TextSingleLine,
            caption);

        p->setPen(ActiveFg);
        p->drawText(
            textRect,
            Qt::AlignCenter | Qt::TextSingleLine,
            caption);

    } else {
        // Oxygen's title renderer paints a light offset contrast pass
        // behind its foreground text. Preserve Human's text placement.
        p->setPen(oxygenLightColor(NormalBg));
        p->drawText(textRect.translated(0, 1),
                    Qt::AlignCenter | Qt::TextSingleLine, caption);
        p->setPen(NormalFg);
        p->drawText(textRect, Qt::AlignCenter | Qt::TextSingleLine, caption);
    }

    m_leftButtons->paint(p, QRectF(0, 0, w, h));
    m_rightButtons->paint(p, QRectF(0, 0, w, h));
}

void Decoration::paint(QPainter *painter,
                       const QRectF &repaintArea)
{
    Q_UNUSED(repaintArea)

    painter->save();

    painter->setRenderHint(
        QPainter::Antialiasing,
        false);

    if (!window()->isMaximized()) {
        const int w = qRound(rect().width());
        const int h = qRound(rect().height());

        painter->setClipRegion(
            humanWindowRegion(w, h),
            Qt::IntersectClip);
    }

    paintFrame(painter);
    paintTitleBar(painter);

    painter->restore();
}

} // namespace KarmicHuman

K_PLUGIN_FACTORY_WITH_JSON(
    KarmicHumanFactory,
    "karmichuman.json",
    registerPlugin<KarmicHuman::Decoration>();
    registerPlugin<KarmicHuman::Button>();
)

#include "karmichuman.moc"
