#ifndef MONSTERITEM_H
#define MONSTERITEM_H

#include <QGraphicsItem>
#include <QPixmap>

class MonsterGraphicsItem : public QGraphicsItem
{
public:
    MonsterGraphicsItem(int gridX, int gridY, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    int gridX() const { return m_gridX; }
    int gridY() const { return m_gridY; }
    void setGridPos(int x, int y);
    void syncScenePos();

private:
    int m_gridX, m_gridY;
    QPixmap m_pixmap;
};

#endif // MONSTERITEM_H
