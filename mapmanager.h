#ifndef MAPMANAGER_H
#define MAPMANAGER_H

#include <QPoint>
#include <QPointF>
#include <QVector>

struct Tile {
    enum Type { Empty = 0, Dirt, Rock, Crystal, Bedrock,
                IronOre, WaterCrystal, DarkCrystal };
    int type = Empty;
    int durability = 0;
    float temperature = 20.0f;
    int resourceAmount = 0;
};

class MapManager
{
public:
    static MapManager& instance();

    void generateMap(int seed = -1);

    Tile getTile(int x, int y) const;
    void setTile(int x, int y, const Tile &t);

    // 挖掘返回实际挖掘量（受限于剩余耐久），返回产出资源类型和数量
    struct DigResult {
        bool success = false;
        int resourceType = -1;  // -1 表示无资源产出
        int resourceAmount = 0;
    };
    DigResult digTile(int x, int y, int amount);

    QVector<QPoint> getSurrounding(int x, int y, int range, bool includeCenter = false) const;
    bool isInBounds(int x, int y) const;
    bool isAdjacent(int x1, int y1, int x2, int y2) const;  // 4-邻接
    int depth() const { return 80; }

    QPointF tileToScenePos(int x, int y) const;
    QPoint scenePosToTile(const QPointF &pos) const;

    static constexpr int TILE_SIZE = 32;
    static constexpr int MAP_WIDTH = 60;
    static constexpr int MAP_DEPTH = 80;

private:
    MapManager() = default;
    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;

    int layerYToDepth(int y) const;   // 返回地层: 0=浅层,1=结晶,2=黑暗核心
    int pickTileType(int depth, bool isBorder) const;

    Tile m_grid[MAP_DEPTH][MAP_WIDTH];
};

inline bool MapManager::isAdjacent(int x1, int y1, int x2, int y2) const
{
    return (qAbs(x1 - x2) + qAbs(y1 - y2)) == 1;
}

#endif // MAPMANAGER_H
