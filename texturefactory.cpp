#include "texturefactory.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

// Tile 类型枚举（与 MapManager 中 Tile::Type 一致）
namespace TileType {
    enum { Empty, Dirt, Rock, Crystal, Bedrock, IronOre, WaterCrystal, DarkCrystal };
}
// Building 类型枚举偏移 100 避免冲突
namespace BldType {
    enum { Storage = 100, Generator, OxygenMaker, Decompressor, GrowPod, PowerPole };
}

TextureFactory& TextureFactory::instance()
{
    static TextureFactory inst;
    return inst;
}

TextureFactory::TextureFactory()
{
    generateAll();
}

void TextureFactory::generateAll()
{
    if (m_generated) return;
    generateTilePixmaps();
    generateBuildingPixmaps();
    // 人物 — 金色圆形 + 眼睛
    {
        QPixmap pm(PX, PX);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#FFD700"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 2, 24, 24);
        p.setBrush(QColor("#1a1a2e"));
        p.drawEllipse(10, 8, 5, 6);
        p.drawEllipse(18, 8, 5, 6);
        // 身体
        p.setBrush(QColor("#FFD700"));
        p.drawRoundedRect(8, 20, 16, 10, 3, 3);
        p.end();
        m_playerPixmap = pm;
    }
    // 怪物 — 橙红三角形 + 眼
    {
        QPixmap pm(PX, PX);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QPolygonF tri;
        tri << QPointF(16, 2) << QPointF(4, 28) << QPointF(28, 28);
        p.setBrush(QColor("#FF4500"));
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);
        p.setBrush(QColor("#1a1a2e"));
        p.drawEllipse(10, 16, 5, 5);
        p.drawEllipse(18, 16, 5, 5);
        p.end();
        m_monsterPixmap = pm;
    }
    m_generated = true;
}

void TextureFactory::generateTilePixmaps()
{
    struct Def { int type; QColor base; int style; };
    // style: 0=noise fill, 1=crack, 2=diamond, 3=solid, 4=noise+rust, 5=droplet, 6=star
    QList<Def> defs = {
        {TileType::Empty,        QColor("#2F2F2F"), 3},
        {TileType::Dirt,         QColor("#8B6914"), 0},
        {TileType::Rock,         QColor("#696969"), 1},
        {TileType::Crystal,      QColor("#00CED1"), 2},
        {TileType::Bedrock,      QColor("#1a1a2e"), 3},
        {TileType::IronOre,      QColor("#B22222"), 4},
        {TileType::WaterCrystal, QColor("#4169E1"), 5},
        {TileType::DarkCrystal,  QColor("#9400D3"), 6},
    };

    const QRect r(0, 0, PX, PX);
    for (const auto &d : defs) {
        QPixmap pm(PX, PX);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        switch (d.style) {
        case 0: drawNoise(p, r, d.base); break;
        case 1: p.fillRect(r, d.base); drawCrack(p, r); break;
        case 2: p.fillRect(r, d.base.darker(150)); drawDiamond(p, r, d.base); break;
        case 3: p.fillRect(r, d.base); break;
        case 4: drawNoise(p, r, QColor("#8B6914")); drawNoise(p, r, d.base, 25); break;
        case 5: p.fillRect(r, d.base.darker(150)); drawDroplet(p, r, d.base); break;
        case 6: p.fillRect(r, d.base.darker(150)); drawStar(p, r, d.base); break;
        }
        p.end();
        m_cache[d.type] = pm;
    }
}

void TextureFactory::generateBuildingPixmaps()
{
    struct BDef { int type; QColor bg; QColor fg; QString label; };
    QList<BDef> bdefs = {
        {BldType::Storage,       QColor("#5C4033"), QColor("#DEB887"), "储"},
        {BldType::Generator,     QColor("#4A4A4A"), QColor("#FF6347"), "电"},
        {BldType::OxygenMaker,   QColor("#2E4A62"), QColor("#87CEEB"), "氧"},
        {BldType::Decompressor,  QColor("#3D5C3A"), QColor("#90EE90"), "压"},
        {BldType::GrowPod,       QColor("#4A3A2A"), QColor("#7CFC00"), "植"},
        {BldType::PowerPole,     QColor("#696969"), QColor("#FFD700"), "线"},
    };
    const QRect r(0, 0, PX, PX);
    for (const auto &b : bdefs) {
        QPixmap pm(PX, PX);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(r.adjusted(2, 2, -2, -2), b.bg);
        p.setPen(QPen(b.fg, 2));
        p.drawRect(r.adjusted(3, 3, -3, -3));
        QFont f = p.font();
        f.setPixelSize(14);
        f.setBold(true);
        p.setFont(f);
        p.setPen(b.fg);
        p.drawText(r, Qt::AlignCenter, b.label);
        p.end();
        m_cache[b.type] = pm;
    }
}

QPixmap TextureFactory::tilePixmap(int tileType)
{
    return m_cache.value(tileType);
}

QPixmap TextureFactory::buildingPixmap(int buildingType)
{
    return m_cache.value(buildingType);
}

QPixmap TextureFactory::playerPixmap() { return m_playerPixmap; }
QPixmap TextureFactory::monsterPixmap() { return m_monsterPixmap; }

// === 绘制辅助函数 ===

void TextureFactory::drawNoise(QPainter &p, const QRect &r, const QColor &base, int density)
{
    p.fillRect(r, base.darker(110));
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < density; i++) {
        int rx = rng->bounded(r.width());
        int ry = rng->bounded(r.height());
        int v = rng->bounded(40);
        QColor c = base.lighter(100 + v);
        p.setPen(c);
        p.drawPoint(r.x() + rx, r.y() + ry);
    }
}

void TextureFactory::drawCrack(QPainter &p, const QRect &r)
{
    p.setPen(QPen(QColor("#3A3A3A"), 1));
    p.drawLine(r.x()+4, r.y()+6, r.x()+14, r.y()+16);
    p.drawLine(r.x()+14, r.y()+16, r.x()+20, r.y()+12);
    p.drawLine(r.x()+14, r.y()+16, r.x()+10, r.y()+26);
    p.drawLine(r.x()+22, r.y()+4, r.x()+26, r.y()+14);
}

void TextureFactory::drawDiamond(QPainter &p, const QRect &r, const QColor &c)
{
    QPolygonF diamond;
    diamond << QPointF(r.center().x(), r.top()+4)
            << QPointF(r.right()-4, r.center().y())
            << QPointF(r.center().x(), r.bottom()-4)
            << QPointF(r.left()+4, r.center().y());
    p.setBrush(c.lighter(130));
    p.setPen(Qt::NoPen);
    p.drawPolygon(diamond);
}

void TextureFactory::drawDroplet(QPainter &p, const QRect &r, const QColor &c)
{
    p.setBrush(c.lighter(140));
    p.setPen(Qt::NoPen);
    QPainterPath path;
    path.moveTo(r.center().x(), r.top()+4);
    path.cubicTo(r.right()-2, r.top()+10, r.right()-2, r.bottom()-6, r.center().x(), r.bottom()-2);
    path.cubicTo(r.left()+2, r.bottom()-6, r.left()+2, r.top()+10, r.center().x(), r.top()+4);
    p.drawPath(path);
}

void TextureFactory::drawStar(QPainter &p, const QRect &r, const QColor &c)
{
    p.setBrush(c.lighter(150));
    p.setPen(Qt::NoPen);
    QPolygonF star;
    const int cx = r.center().x(), cy = r.center().y();
    for (int i = 0; i < 8; i++) {
        qreal angle = i * M_PI / 4.0 - M_PI / 2.0;
        qreal rad = (i % 2 == 0) ? 12.0 : 5.0;
        star << QPointF(cx + rad * cos(angle), cy + rad * sin(angle));
    }
    p.drawPolygon(star);
}
