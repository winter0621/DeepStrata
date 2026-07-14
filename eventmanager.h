#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <QObject>
#include <QTimer>
#include <QPoint>
#include <QList>

struct Monster {
    int gridX, gridY;
    int hp = 3;
    float moveTimer = 2.0f;
};

class EventManager : public QObject
{
    Q_OBJECT
public:
    static EventManager& instance();

    void start();
    void stop();
    void reset();

    QList<Monster>& monsters() { return m_monsters; }
    const QList<Monster>& cmonsters() const { return m_monsters; }

    int gameDay() const;
    QString gameTime() const;
    int elapsedSeconds() const { return m_elapsedSeconds; }
    void setElapsedSeconds(int s) { m_elapsedSeconds = s; }

    void setPlayerPos(int x, int y) { m_playerX = x; m_playerY = y; }
    int playerX() const { return m_playerX; }
    int playerY() const { return m_playerY; }

signals:
    void monsterSpawned(QPoint pos);
    void monsterDied(QPoint pos);
    void pressureBreak(QPoint pos);

private slots:
    void tick();

private:
    EventManager();
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    void checkTemperatureEvent();
    void checkPressureEvent();
    void checkMonsterSpawn();
    void updateMonsters(float dt);
    void checkCrystalGrowth();

    QTimer m_timer;
    QList<Monster> m_monsters;
    int m_elapsedSeconds = 0;
    int m_monsterSpawnCooldown = 0;
    int m_playerX = 30, m_playerY = 2;
    int m_secondCounter = 0; // 帧计数器，每秒执行一次事件
};

#endif // EVENTMANAGER_H
