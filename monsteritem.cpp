#include "monsteritem.h"
#include "texturefactory.h"
#include "mapmanager.h"
#include <QPainter>

MonsterGraphicsItem::MonsterGraphicsItem(int gridX, int gridY, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_gridX(gridX), m_gridY(gridY)
{
    m_pixmap = TextureFactory::instance().monsterPixmap();
    setZValue(8);
    syncScenePos();
}

QRectF MonsterGraphicsItem::boundingRect() const
{
    return QRectF(0, 0, MapManager::TILE_SIZE, MapManager::TILE_SIZE);
}

void MonsterGraphicsItem::paint(QPainter *painter,
                                 const QStyleOptionGraphicsItem *,
                                 QWidget *)
{
    painter->drawPixmap(0, 0, m_pixmap);
}

void MonsterGraphicsItem::setGridPos(int x, int y)
{
    m_gridX = x;
    m_gridY = y;
    syncScenePos();
}

void MonsterGraphicsItem::syncScenePos()
{
    QPointF sp = MapManager::instance().tileToScenePos(m_gridX, m_gridY);
    setPos(sp);
}
