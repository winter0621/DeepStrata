#include "eventmanager.h"
#include "mapmanager.h"
#include <QRandomGenerator>
#include <QtMath>

EventManager& EventManager::instance()
{
    static EventManager inst;
    return inst;
}

EventManager::EventManager()
{
    connect(&m_timer, &QTimer::timeout, this, &EventManager::tick);
}

void EventManager::start()
{
    m_timer.start(1000 / 60); // 60 FPS
}

void EventManager::stop()
{
    m_timer.stop();
}

void EventManager::reset()
{
    m_monsters.clear();
    m_elapsedSeconds = 0;
    m_monsterSpawnCooldown = 0;
    m_secondCounter = 0;
}

int EventManager::gameDay() const
{
    return m_elapsedSeconds / 480 + 1; // 480秒=1天（游戏加速）
}

QString EventManager::gameTime() const
{
    int day = gameDay();
    int secInDay = m_elapsedSeconds % 480;
    int h = secInDay / 20;  // 480秒映射到24小时
    int m = (secInDay % 20) * 3;
    return QString("Day %1  %2:%3")
        .arg(day)
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'));
}

void EventManager::tick()
{
    m_secondCounter++;
    float dt = 1.0f / 60.0f;

    // 每秒执行一次事件逻辑
    if (m_secondCounter % 60 == 0) {
        m_elapsedSeconds++;
        checkTemperatureEvent();
        checkPressureEvent();
        checkMonsterSpawn();
        checkCrystalGrowth();
    }

    // 怪物AI每帧更新
    updateMonsters(dt);
}

void EventManager::checkTemperatureEvent()
{
    auto &mm = MapManager::instance();
    auto *rng = QRandomGenerator::global();
    int depth = m_playerY;
    float prob = (depth > 40) ? 0.12f : 0.08f;
    if (rng->generateDouble() < prob) {
        int x = rng->bounded(MapManager::MAP_WIDTH);
        int y = rng->bounded(10, MapManager::MAP_DEPTH);
        Tile t = mm.getTile(x, y);
        if (t.type != Tile::Empty && t.type != Tile::Bedrock) {
            float delta = (rng->bounded(2) == 0) ? 3.0f : -3.0f;
            t.temperature += delta;
            mm.setTile(x, y, t);
        }
    }
}

void EventManager::checkPressureEvent()
{
    if (m_playerY <= 40) return;
    auto &mm = MapManager::instance();
    auto *rng = QRandomGenerator::global();

    // 随机选几个玩家附近的格子检查
    for (int attempt = 0; attempt < 3; attempt++) {
        int x = rng->bounded(MapManager::MAP_WIDTH);
        int y = rng->bounded(40, MapManager::MAP_DEPTH);
        Tile t = mm.getTile(x, y);
        if (t.type == Tile::Rock && t.durability > 0) {
            // 找相邻 Empty 格
            auto neighbors = mm.getSurrounding(x, y, 1);
            for (const auto &n : neighbors) {
                Tile nt = mm.getTile(n.x(), n.y());
                if (nt.type == Tile::Empty && rng->generateDouble() < 0.2) {
                    // 地层破损：Empty 格被填 Dirt
                    Tile fill;
                    fill.type = Tile::Dirt;
                    fill.durability = 1;
                    fill.temperature = t.temperature;
                    mm.setTile(n.x(), n.y(), fill);
                    emit pressureBreak(n);
                    return;
                }
            }
        }
    }
}

void EventManager::checkMonsterSpawn()
{
    if (m_playerY < 50) return;
    if (m_monsters.size() >= 3) return;
    if (m_monsterSpawnCooldown > 0) {
        m_monsterSpawnCooldown--;
        return;
    }

    auto *rng = QRandomGenerator::global();
    if (rng->generateDouble() < 0.3) {
        // 在玩家周围随机空位生成怪物
        auto &mm = MapManager::instance();
        for (int attempt = 0; attempt < 20; attempt++) {
            int dx = rng->bounded(-8, 9);
            int dy = rng->bounded(-8, 9);
            int sx = m_playerX + dx, sy = m_playerY + dy;
            if (!mm.isInBounds(sx, sy)) continue;
            Tile t = mm.getTile(sx, sy);
            if (t.type == Tile::Empty) {
                Monster m;
                m.gridX = sx;
                m.gridY = sy;
                m.hp = 3;
                m.moveTimer = 2.0f;
                m_monsters.append(m);
                emit monsterSpawned(QPoint(sx, sy));
                m_monsterSpawnCooldown = 30 + rng->bounded(31); // 30-60秒
                return;
            }
        }
    }
}

void EventManager::updateMonsters(float dt)
{
    auto &mm = MapManager::instance();
    for (int i = m_monsters.size() - 1; i >= 0; i--) {
        Monster &mon = m_monsters[i];
        int dist = qAbs(mon.gridX - m_playerX) + qAbs(mon.gridY - m_playerY);

        if (dist <= 1) {
            // 接触伤害已由 MainWindow 处理
            continue;
        }

        if (dist > 5) continue; // 超出追踪范围不动

        mon.moveTimer -= dt;
        if (mon.moveTimer <= 0) {
            mon.moveTimer = 2.0f;
            // 曼哈顿贪心移动
            int bestDx = 0, bestDy = 0;
            int bestDist = dist;
            const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (int d = 0; d < 4; d++) {
                int nx = mon.gridX + dirs[d][0];
                int ny = mon.gridY + dirs[d][1];
                if (!mm.isInBounds(nx, ny)) continue;
                Tile t = mm.getTile(nx, ny);
                if (t.type != Tile::Empty) continue;
                // 不能移到另一个怪物所在格
                bool occupied = false;
                for (int j = 0; j < m_monsters.size(); j++) {
                    if (i != j && m_monsters[j].gridX == nx && m_monsters[j].gridY == ny) {
                        occupied = true; break;
                    }
                }
                if (occupied) continue;
                int nd = qAbs(nx - m_playerX) + qAbs(ny - m_playerY);
                if (nd < bestDist) { bestDist = nd; bestDx = dirs[d][0]; bestDy = dirs[d][1]; }
            }
            mon.gridX += bestDx;
            mon.gridY += bestDy;
        }
    }
}

void EventManager::checkCrystalGrowth()
{
    auto &mm = MapManager::instance();
    auto *rng = QRandomGenerator::global();
    if (rng->generateDouble() < 0.02) {
        // 结晶层（26-55）的 Empty 格小概率生长水晶
        int x = rng->bounded(MapManager::MAP_WIDTH);
        int y = rng->bounded(26, 56);
        Tile t = mm.getTile(x, y);
        if (t.type == Tile::Empty) {
            t.type = Tile::Crystal;
            t.durability = 3;
            t.temperature = 35.0f;
            t.resourceAmount = 1;
            mm.setTile(x, y, t);
        }
    }
}
