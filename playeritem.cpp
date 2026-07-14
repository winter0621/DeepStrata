#include "playeritem.h"
#include "texturefactory.h"
#include "mapmanager.h"
#include <QPainter>

PlayerGraphicsItem::PlayerGraphicsItem(QGraphicsItem *parent)
    : QGraphicsItem(parent), m_gridX(30), m_gridY(2)
{
    m_pixmap = TextureFactory::instance().playerPixmap();
    setZValue(10);
    syncScenePos();
}

QRectF PlayerGraphicsItem::boundingRect() const
{
    return QRectF(0, 0, MapManager::TILE_SIZE, MapManager::TILE_SIZE);
}

void PlayerGraphicsItem::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem *,
                                QWidget *)
{
    painter->drawPixmap(0, 0, m_pixmap);
}

void PlayerGraphicsItem::setGridPos(int x, int y)
{
    m_gridX = x;
    m_gridY = y;
    syncScenePos();
}

bool PlayerGraphicsItem::tryMove(int dx, int dy)
{
    int nx = m_gridX + dx;
    int ny = m_gridY + dy;
    auto &mm = MapManager::instance();
    if (!mm.isInBounds(nx, ny)) return false;
    Tile t = mm.getTile(nx, ny);
    // 只有 Empty 格可通过
    if (t.type != Tile::Empty) return false;
    m_gridX = nx;
    m_gridY = ny;
    syncScenePos();
    return true;
}

void PlayerGraphicsItem::syncScenePos()
{
    QPointF sp = MapManager::instance().tileToScenePos(m_gridX, m_gridY);
    setPos(sp);
}
