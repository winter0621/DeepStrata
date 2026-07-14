#include "mapmanager.h"
#include <QRandomGenerator>
#include <QtMath>

MapManager& MapManager::instance()
{
    static MapManager inst;
    return inst;
}

void MapManager::generateMap(int seed)
{
    auto *rng = QRandomGenerator::global();
    if (seed >= 0) rng->seed(static_cast<quint32>(seed));

    for (int y = 0; y < MAP_DEPTH; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            Tile t;
            int layer = layerYToDepth(y);
            bool border = (x == 0 || x == MAP_WIDTH - 1);

            if (y == 0) {
                // 地表行：全部空
                t.type = Tile::Empty;
                t.durability = 0;
                t.temperature = 20.0f;
            } else if (y == MAP_DEPTH - 1 || border) {
                // 底行和左右边界：基岩
                t.type = Tile::Bedrock;
                t.durability = 99;
                t.temperature = 60.0f;
            } else {
                t.type = pickTileType(layer, false);

                // 基础温度按地层
                switch (layer) {
                case 0: t.temperature = 18.0f + rng->bounded(5); break;
                case 1: t.temperature = 32.0f + rng->bounded(7); break;
                case 2: t.temperature = 55.0f + rng->bounded(11); break;
                }

                // 耐久度
                switch (t.type) {
                case Tile::Empty:    t.durability = 0; break;
                case Tile::Dirt:     t.durability = 2; break;
                case Tile::Rock:     t.durability = 4; break;
                case Tile::Crystal:  t.durability = 5; break;
                case Tile::Bedrock:  t.durability = 99; break;
                case Tile::IronOre:      t.durability = 3; t.resourceAmount = 1 + rng->bounded(3); break;
                case Tile::WaterCrystal: t.durability = 4; t.resourceAmount = 1 + rng->bounded(2); break;
                case Tile::DarkCrystal:  t.durability = 5; t.resourceAmount = 1 + rng->bounded(2); break;
                }
            }
            m_grid[y][x] = t;
        }
    }

    // 玩家出生点确保空
    m_grid[2][30].type = Tile::Empty;
    m_grid[2][30].durability = 0;
    m_grid[2][29].type = Tile::Empty;
    m_grid[2][29].durability = 0;
    m_grid[2][31].type = Tile::Empty;
    m_grid[2][31].durability = 0;
    m_grid[3][30].type = Tile::Empty;
    m_grid[3][30].durability = 0;
}

Tile MapManager::getTile(int x, int y) const
{
    if (!isInBounds(x, y)) return Tile();
    return m_grid[y][x];
}

void MapManager::setTile(int x, int y, const Tile &t)
{
    if (isInBounds(x, y)) m_grid[y][x] = t;
}

MapManager::DigResult MapManager::digTile(int x, int y, int amount)
{
    DigResult res;
    if (!isInBounds(x, y)) return res;

    Tile &t = m_grid[y][x];
    if (t.durability <= 0 || t.type == Tile::Bedrock) return res;

    int dug = qMin(amount, t.durability);
    t.durability -= dug;
    res.success = true;

    if (t.durability <= 0) {
        // 产出资源
        if (t.resourceAmount > 0) {
            res.resourceType = t.type;
            res.resourceAmount = t.resourceAmount;
        }
        t.type = Tile::Empty;
        t.resourceAmount = 0;
    }
    return res;
}

QVector<QPoint> MapManager::getSurrounding(int x, int y, int range, bool includeCenter) const
{
    QVector<QPoint> pts;
    for (int dy = -range; dy <= range; dy++) {
        for (int dx = -range; dx <= range; dx++) {
            if (!includeCenter && dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (isInBounds(nx, ny))
                pts.append(QPoint(nx, ny));
        }
    }
    return pts;
}

bool MapManager::isInBounds(int x, int y) const
{
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_DEPTH;
}

QPointF MapManager::tileToScenePos(int x, int y) const
{
    return QPointF(x * TILE_SIZE, y * TILE_SIZE);
}

QPoint MapManager::scenePosToTile(const QPointF &pos) const
{
    int x = static_cast<int>(pos.x()) / TILE_SIZE;
    int y = static_cast<int>(pos.y()) / TILE_SIZE;
    return QPoint(qBound(0, x, MAP_WIDTH - 1), qBound(0, y, MAP_DEPTH - 1));
}

int MapManager::layerYToDepth(int y) const
{
    if (y <= 25) return 0;
    if (y <= 55) return 1;
    return 2;
}

int MapManager::pickTileType(int depth, bool /*border*/) const
{
    auto *rng = QRandomGenerator::global();
    int r = rng->bounded(100);

    switch (depth) {
    case 0: // 浅层岩土: Dirt 60%, Rock 30%, IronOre 8%, Empty 2%
        if (r < 60) return Tile::Dirt;
        if (r < 90) return Tile::Rock;
        if (r < 98) return Tile::IronOre;
        return Tile::Empty;
    case 1: // 结晶层: Rock 45%, Crystal 30%, Dirt 15%, WaterCrystal 8%, Empty 2%
        if (r < 45) return Tile::Rock;
        if (r < 75) return Tile::Crystal;
        if (r < 90) return Tile::Dirt;
        if (r < 98) return Tile::WaterCrystal;
        return Tile::Empty;
    case 2: // 黑暗核心: Bedrock 35%, Crystal 30%, Rock 25%, DarkCrystal 8%, Empty 2%
        if (r < 35) return Tile::Bedrock;
        if (r < 65) return Tile::Crystal;
        if (r < 90) return Tile::Rock;
        if (r < 98) return Tile::DarkCrystal;
        return Tile::Empty;
    }
    return Tile::Dirt;
}
