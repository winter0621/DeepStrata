#include "buildingitem.h"
#include "texturefactory.h"
#include "mapmanager.h"
#include <QPainter>

BuildingGraphicsItem::BuildingGraphicsItem(const Building &b, QGraphicsItem *parent)
    : QGraphicsRectItem(b.gridX * MapManager::TILE_SIZE,
                        b.gridY * MapManager::TILE_SIZE,
                        b.width  * MapManager::TILE_SIZE,
                        b.height * MapManager::TILE_SIZE, parent),
      m_key(b.gridX, b.gridY), m_type(b.type)
{
    setZValue(5);
    refreshVisual();
}

void BuildingGraphicsItem::refreshVisual()
{
    const Building *b = BuildManager::instance().buildingAt(m_key.x(), m_key.y());
    if (!b) return;

    QPixmap icon = TextureFactory::instance().buildingPixmap(100 + static_cast<int>(m_type));
    QPixmap scaled = icon.scaled(static_cast<int>(rect().width()),
                                  static_cast<int>(rect().height()),
                                  Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if (!b->powered && Building::powerFor(m_type) != 0 && Building::powerFor(m_type) != 50) {
        // 耗电建筑未通电：红色遮罩
        QPixmap overlay(scaled.size());
        overlay.fill(Qt::transparent);
        QPainter p(&overlay);
        p.drawPixmap(0, 0, scaled);
        p.fillRect(overlay.rect(), QColor(255, 0, 0, 80));
        p.end();
        setBrush(overlay);
    } else {
        setBrush(scaled);
    }
    setPen(Qt::NoPen);
}
