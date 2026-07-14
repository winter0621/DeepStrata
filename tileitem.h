#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsRectItem>
#include "mapmanager.h"

class TileGraphicsItem : public QGraphicsRectItem
{
public:
    TileGraphicsItem(int gridX, int gridY, QGraphicsItem *parent = nullptr);

    int gridX() const { return m_gridX; }
    int gridY() const { return m_gridY; }
    void refreshPixmap();

private:
    int m_gridX, m_gridY;
};

#endif // TILEITEM_H
