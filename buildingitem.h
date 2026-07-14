#ifndef BUILDINGITEM_H
#define BUILDINGITEM_H

#include <QGraphicsRectItem>
#include "buildmanager.h"

class BuildingGraphicsItem : public QGraphicsRectItem
{
public:
    BuildingGraphicsItem(const Building &b, QGraphicsItem *parent = nullptr);
    void refreshVisual();

    QPoint gridKey() const { return m_key; }

private:
    QPoint m_key;
    Building::Type m_type;
};

#endif // BUILDINGITEM_H
