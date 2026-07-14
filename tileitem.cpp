#include "tileitem.h"
#include "texturefactory.h"
#include <QPainter>

TileGraphicsItem::TileGraphicsItem(int gridX, int gridY, QGraphicsItem *parent)
    : QGraphicsRectItem(gridX * MapManager::TILE_SIZE,
                        gridY * MapManager::TILE_SIZE,
                        MapManager::TILE_SIZE,
                        MapManager::TILE_SIZE, parent),
      m_gridX(gridX), m_gridY(gridY)
{
    refreshPixmap();
}

void TileGraphicsItem::refreshPixmap()
{
    Tile t = MapManager::instance().getTile(m_gridX, m_gridY);
    QPixmap pm = TextureFactory::instance().tilePixmap(t.type);

    if (t.durability > 0 && t.type != Tile::Empty && t.type != Tile::Bedrock) {
        // 被部分挖掘的格子叠加裂纹透明度
        QPixmap overlay(pm.size());
        overlay.fill(Qt::transparent);
        QPainter p(&overlay);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, pm);
        // 根据剩余比例加暗
        float ratio = static_cast<float>(t.durability) / 5.0f;
        QColor mask(0, 0, 0, static_cast<int>((1.0f - ratio) * 100));
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.fillRect(overlay.rect(), mask);
        p.end();
        setBrush(overlay);
    } else {
        setBrush(pm);
    }
    setPen(Qt::NoPen);
}
