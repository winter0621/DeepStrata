#ifndef TEXTUREFACTORY_H
#define TEXTUREFACTORY_H

#include <QPixmap>
#include <QHash>

class TextureFactory
{
public:
    static TextureFactory& instance();

    QPixmap tilePixmap(int tileType);
    QPixmap buildingPixmap(int buildingType);
    QPixmap playerPixmap();
    QPixmap monsterPixmap();

    static constexpr int PX = 32;  // 纹理尺寸

private:
    TextureFactory();
    TextureFactory(const TextureFactory&) = delete;
    TextureFactory& operator=(const TextureFactory&) = delete;

    void generateAll();
    void generateTilePixmaps();
    void generateBuildingPixmaps();
    // 绘制辅助
    void drawNoise(QPainter &p, const QRect &r, const QColor &base, int density = 60);
    void drawCrack(QPainter &p, const QRect &r);
    void drawDiamond(QPainter &p, const QRect &r, const QColor &c);
    void drawDroplet(QPainter &p, const QRect &r, const QColor &c);
    void drawStar(QPainter &p, const QRect &r, const QColor &c);

    QHash<int, QPixmap> m_cache;
    QPixmap m_playerPixmap;
    QPixmap m_monsterPixmap;
    bool m_generated = false;
};

#endif // TEXTUREFACTORY_H
