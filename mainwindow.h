#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QHash>
#include <QSet>
#include <QElapsedTimer>

class PlayerGraphicsItem;
class TileGraphicsItem;
class MonsterGraphicsItem;
class BuildingGraphicsItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void gameLoop();
    void onNewGame();
    void onSaveGame();
    void onLoadGame();
    void onToggleFullscreen();
    void onBuildButtonClicked(int id);
    void onCancelBuild();
    void onMonsterSpawned(QPoint pos);
    void onMonsterDied(QPoint pos);
    void onDeath(const QString &reason);
    void onBuildingPlaced(QPoint pos);
    void onBuildingRemoved(QPoint pos);

private:
    void setupUI();
    void setupMenuBar();
    void setupScene();
    void setupConnections();

    void startNewGame();
    void doSave(const QString &filepath);
    void doLoad(const QString &filepath);

    void updateHUD();
    void updateTileVisibility();
    void updateMonsterItems();
    void updateBuildingItems();
    void refreshAllBuildingVisuals();

    void performDig();
    void performBuild();
    void performRepair();
    QString resourceTypeToString(int tileType) const;

    // 场景
    QGraphicsScene *m_scene;
    QGraphicsView  *m_view;

    // 游戏对象
    PlayerGraphicsItem *m_player = nullptr;
    QHash<QPoint, TileGraphicsItem*> m_tileItems;
    QHash<QPoint, MonsterGraphicsItem*> m_monsterItems;
    QHash<QPoint, BuildingGraphicsItem*> m_buildingItems;

    // 定时器
    QTimer m_gameTimer;
    QElapsedTimer m_frameClock;

    // 键盘状态
    QSet<int> m_keysHeld;

    // 建造状态
    int m_selectedBuilding = -1;

    // HUD 控件
    QLabel *m_lblCrystal, *m_lblIronOre, *m_lblWaterCrystal;
    QLabel *m_lblDarkCrystal, *m_lblFood, *m_lblPower;

    QProgressBar *m_barOxygen, *m_barTemp, *m_barFatigue, *m_barRadiation;
    QLabel *m_lblOxygenStatus, *m_lblTempStatus, *m_lblFatigueStatus, *m_lblRadiationStatus;
    QLabel *m_lblOverallStatus, *m_lblGameTime, *m_lblDepth, *m_lblLayer;

    // 工作状态按钮
    QButtonGroup *m_workGroup;
    QRadioButton *m_rbIdle, *m_rbMining, *m_rbBuilding, *m_rbRepairing;

    // 游戏状态
    bool m_gameRunning = false;
    bool m_gameOver = false;
    float m_moveCooldown = 0;
    float m_actionCooldown = 0;
    float m_monsterDamageCooldown = 0;
};

#endif // MAINWINDOW_H
