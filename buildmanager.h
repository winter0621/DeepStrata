#ifndef BUILDMANAGER_H
#define BUILDMANAGER_H

#include <QObject>
#include <QHash>
#include <QPoint>
#include <QVector>

struct Building {
    enum Type { Storage = 0, Generator, OxygenMaker, Decompressor,
                GrowPod, PowerPole };
    Type type;
    int gridX, gridY;
    int width, height;
    bool powered;
    bool active;

    static int powerFor(Type t);
    static void buildingSize(Type t, int &w, int &h);
    static const char* typeName(Type t);
};

class BuildManager : public QObject
{
    Q_OBJECT
public:
    static BuildManager& instance();

    bool canPlace(int x, int y, Building::Type type) const;
    bool placeBuilding(int x, int y, Building::Type type);
    void removeBuilding(int x, int y);

    QVector<const Building*> getBuildingsInRadius(int cx, int cy, int range) const;
    const Building* buildingAt(int x, int y) const;
    QHash<QPoint, Building> buildings() const { return m_buildings; }

    void updatePowerGrid();
    int totalPower() const { return m_totalPower; }
    int usedPower() const   { return m_usedPower; }

    // 资源系统
    int resource(const QString &key) const;
    void addResource(const QString &key, int amount);
    bool consumeResource(const QString &key, int amount);

    // 存档支持
    void restoreBuilding(const Building &b);
    void clearResources();

signals:
    void buildingPlaced(QPoint pos);
    void buildingRemoved(QPoint pos);
    void resourcesChanged();

private:
    BuildManager() = default;
    BuildManager(const BuildManager&) = delete;
    BuildManager& operator=(const BuildManager&) = delete;

    bool checkPlacementOverlap(int x, int y, int w, int h, int ignoreKey = -1) const;
    bool canAfford(Building::Type type) const;
    void deductCost(Building::Type type);

    QHash<QPoint, Building> m_buildings; // key = gridX*1000+gridY 的 QPoint
    int m_totalPower = 0;
    int m_usedPower = 0;
    QHash<QString, int> m_resources;     // "crystal", "ironOre", "waterCrystal", "darkCrystal", "food"
};

#endif // BUILDMANAGER_H
