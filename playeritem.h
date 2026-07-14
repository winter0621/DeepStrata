#ifndef PLAYERITEM_H
#define PLAYERITEM_H

#include <QGraphicsItem>
#include <QPixmap>

class PlayerGraphicsItem : public QGraphicsItem
{
public:
    PlayerGraphicsItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    int gridX() const { return m_gridX; }
    int gridY() const { return m_gridY; }
    void setGridPos(int x, int y);
    bool tryMove(int dx, int dy);  // 返回是否成功移动
    void syncScenePos();           // 格坐标→场景像素坐标

private:
    int m_gridX, m_gridY;
    QPixmap m_pixmap;
};

#endif // PLAYERITEM_H
