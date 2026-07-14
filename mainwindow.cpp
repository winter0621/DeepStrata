#include "mainwindow.h"
#include "mapmanager.h"
#include "buildmanager.h"
#include "survivedata.h"
#include "eventmanager.h"
#include "texturefactory.h"
#include "tileitem.h"
#include "playeritem.h"
#include "monsteritem.h"
#include "buildingitem.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>
#include <QRandomGenerator>
#include <QPainter>
#include <QtMath>

// =====================================================================
// Part 1: 初始化和UI搭建
// =====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 确保纹理工厂已初始化
    TextureFactory::instance();

    setupUI();
    setupMenuBar();
    setupConnections();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    // 中央部件：水平分割
    QWidget *central = new QWidget(this);
    QHBoxLayout *hlay = new QHBoxLayout(central);
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->setSpacing(0);

    // 左侧：游戏场景
    setupScene();
    hlay->addWidget(m_view, 1);

    // 右侧面板
    QWidget *rightPanel = new QWidget;
    rightPanel->setFixedWidth(240);
    rightPanel->setStyleSheet("background-color: #1e1e2e; color: #cdd6f4;");
    QVBoxLayout *rlay = new QVBoxLayout(rightPanel);
    rlay->setContentsMargins(8, 8, 8, 8);
    rlay->setSpacing(6);

    // === 资源面板 ===
    QGroupBox *resGrp = new QGroupBox(QString::fromUtf8("资源"));
    resGrp->setStyleSheet("QGroupBox { color: #f5c2e7; font-weight: bold; border: 1px solid #45475a; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *resLay = new QVBoxLayout(resGrp);
    m_lblPower = new QLabel;
    m_lblCrystal = new QLabel;
    m_lblIronOre = new QLabel;
    m_lblWaterCrystal = new QLabel;
    m_lblDarkCrystal = new QLabel;
    m_lblFood = new QLabel;
    resLay->addWidget(m_lblPower);
    resLay->addWidget(m_lblCrystal);
    resLay->addWidget(m_lblIronOre);
    resLay->addWidget(m_lblWaterCrystal);
    resLay->addWidget(m_lblDarkCrystal);
    resLay->addWidget(m_lblFood);

    // === 生存状态面板 ===
    QGroupBox *survGrp = new QGroupBox(QString::fromUtf8("生存状态"));
    survGrp->setStyleSheet("QGroupBox { color: #f5c2e7; font-weight: bold; border: 1px solid #45475a; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *survLay = new QVBoxLayout(survGrp);

    auto makeBarRow = [&](const QString &label, QProgressBar *&bar, QLabel *&statusLbl) {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setFixedWidth(30);
        row->addWidget(lbl);
        bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(12);
        row->addWidget(bar, 1);
        statusLbl = new QLabel;
        statusLbl->setFixedWidth(32);
        row->addWidget(statusLbl);
        survLay->addLayout(row);
    };
    makeBarRow("O2",  m_barOxygen,    m_lblOxygenStatus);
    makeBarRow(QString::fromUtf8("温"), m_barTemp,       m_lblTempStatus);
    makeBarRow(QString::fromUtf8("疲"), m_barFatigue,     m_lblFatigueStatus);
    makeBarRow(QString::fromUtf8("辐"), m_barRadiation,   m_lblRadiationStatus);

    m_lblOverallStatus = new QLabel;
    m_lblOverallStatus->setAlignment(Qt::AlignCenter);
    m_lblOverallStatus->setStyleSheet("font-weight: bold; font-size: 14px;");
    survLay->addWidget(m_lblOverallStatus);

    // === 工作状态 ===
    QGroupBox *workGrp = new QGroupBox(QString::fromUtf8("工作状态"));
    workGrp->setStyleSheet("QGroupBox { color: #f5c2e7; font-weight: bold; border: 1px solid #45475a; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *workLay = new QVBoxLayout(workGrp);
    m_workGroup = new QButtonGroup(this);
    m_workGroup->setExclusive(true);
    m_rbIdle     = new QRadioButton(QString::fromUtf8("0 空闲"));
    m_rbMining   = new QRadioButton(QString::fromUtf8("1 挖掘"));
    m_rbBuilding = new QRadioButton(QString::fromUtf8("2 建造"));
    m_rbRepairing= new QRadioButton(QString::fromUtf8("3 维修"));
    m_workGroup->addButton(m_rbIdle, 0);
    m_workGroup->addButton(m_rbMining, 1);
    m_workGroup->addButton(m_rbBuilding, 2);
    m_workGroup->addButton(m_rbRepairing, 3);
    m_rbIdle->setChecked(true);
    for (auto *rb : {m_rbIdle, m_rbMining, m_rbBuilding, m_rbRepairing}) {
        rb->setStyleSheet("color: #cdd6f4;");
        workLay->addWidget(rb);
    }
    QObject::connect(m_workGroup, &QButtonGroup::buttonClicked,
        [this](QAbstractButton *btn) {
            int id = m_workGroup->id(btn);
            if (id >= 0)
                SurviveData::instance().setWorkState(static_cast<WorkState>(id));
        });

    // === 时间和深度 ===
    m_lblGameTime = new QLabel(QString::fromUtf8("未开始"));
    m_lblDepth    = new QLabel;
    m_lblLayer    = new QLabel;
    QGroupBox *infoGrp = new QGroupBox(QString::fromUtf8("信息"));
    infoGrp->setStyleSheet("QGroupBox { color: #f5c2e7; font-weight: bold; border: 1px solid #45475a; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *infoLay = new QVBoxLayout(infoGrp);
    infoLay->addWidget(m_lblGameTime);
    infoLay->addWidget(m_lblDepth);
    infoLay->addWidget(m_lblLayer);

    // 组装右面板
    rlay->addWidget(resGrp);
    rlay->addWidget(survGrp);
    rlay->addWidget(workGrp);
    rlay->addWidget(infoGrp);

    // 建造面板
    QGroupBox *buildGrp = new QGroupBox(QString::fromUtf8("建造 (热键 E)"));
    buildGrp->setStyleSheet("QGroupBox { color: #f5c2e7; font-weight: bold; border: 1px solid #45475a; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *buildLay = new QVBoxLayout(buildGrp);
    const char* buildNames[] = {"储物箱", "发电机", "制氧机", "减压器", "种植舱", "电线杆"};
    const int buildTypes[] = {Building::Storage, Building::Generator, Building::OxygenMaker,
                              Building::Decompressor, Building::GrowPod, Building::PowerPole};
    for (int i = 0; i < 6; i++) {
        QPushButton *btn = new QPushButton(QString::fromUtf8(buildNames[i]));
        btn->setStyleSheet("QPushButton { background-color: #313244; color: #cdd6f4; padding: 4px; } QPushButton:hover { background-color: #45475a; }");
        int btype = buildTypes[i];
        QObject::connect(btn, &QPushButton::clicked, [this, btype]() {
            onBuildButtonClicked(btype);
        });
        buildLay->addWidget(btn);
    }
    QPushButton *btnCancel = new QPushButton(QString::fromUtf8("取消建造"));
    btnCancel->setStyleSheet("QPushButton { background-color: #e64553; color: #fff; padding: 4px; }");
    QObject::connect(btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelBuild);
    buildLay->addWidget(btnCancel);
    rlay->addWidget(buildGrp);

    rlay->addStretch();

    hlay->addWidget(rightPanel);
    setCentralWidget(central);
}

void MainWindow::setupMenuBar()
{
    QMenu *gameMenu = menuBar()->addMenu(QString::fromUtf8("游戏"));
    gameMenu->addAction(QString::fromUtf8("新游戏 (Ctrl+N)"), QKeySequence("Ctrl+N"), this, &MainWindow::onNewGame);
    gameMenu->addAction(QString::fromUtf8("存档 (Ctrl+S)"), QKeySequence("Ctrl+S"), this, &MainWindow::onSaveGame);
    gameMenu->addAction(QString::fromUtf8("读档 (Ctrl+L)"), QKeySequence("Ctrl+L"), this, &MainWindow::onLoadGame);
    gameMenu->addSeparator();
    gameMenu->addAction(QString::fromUtf8("全屏 (F11)"), QKeySequence("F11"), this, &MainWindow::onToggleFullscreen);
    gameMenu->addSeparator();
    gameMenu->addAction(QString::fromUtf8("退出"), QKeySequence("Ctrl+Q"), this, &QMainWindow::close);
}

void MainWindow::setupScene()
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, MapManager::MAP_WIDTH  * MapManager::TILE_SIZE,
                                MapManager::MAP_DEPTH * MapManager::TILE_SIZE);
    m_scene->setBackgroundBrush(QColor("#11111b"));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view->setFocusPolicy(Qt::StrongFocus);

    // TileItem 在 startNewGame 中按需创建
}

void MainWindow::setupConnections()
{
    // 游戏定时器
    QObject::connect(&m_gameTimer, &QTimer::timeout, this, &MainWindow::gameLoop);

    // EventManager 信号
    auto &em = EventManager::instance();
    QObject::connect(&em, &EventManager::monsterSpawned, this, &MainWindow::onMonsterSpawned);
    QObject::connect(&em, &EventManager::monsterDied,   this, &MainWindow::onMonsterDied);

    // SurviveData 信号
    QObject::connect(&SurviveData::instance(), &SurviveData::death, this, &MainWindow::onDeath);

    // BuildManager 信号
    QObject::connect(&BuildManager::instance(), &BuildManager::buildingPlaced,  this, &MainWindow::onBuildingPlaced);
    QObject::connect(&BuildManager::instance(), &BuildManager::buildingRemoved, this, &MainWindow::onBuildingRemoved);
}

// =====================================================================
// Part 2: 游戏循环和操控
// =====================================================================

void MainWindow::startNewGame()
{
    // 清除旧场景
    m_scene->clear();
    m_tileItems.clear();
    m_monsterItems.clear();
    m_buildingItems.clear();
    m_player = nullptr;

    // 重置所有模块
    MapManager::instance().generateMap(QRandomGenerator::global()->generate());

    // 清空建造管理器
    auto &bm = BuildManager::instance();
    {
        auto keys = bm.buildings().keys();
        for (const auto &k : keys) bm.removeBuilding(k.x(), k.y());
    }
    bm.clearResources();

    SurviveData::instance().reset();
    EventManager::instance().reset();
    EventManager::instance().setPlayerPos(30, 2);

    // 创建玩家
    m_player = new PlayerGraphicsItem;
    m_player->setGridPos(30, 2);
    m_scene->addItem(m_player);

    // 创建初始可见区域的 TileItem
    updateTileVisibility();

    // 设置初始资源
    bm.addResource("crystal", 5);
    bm.addResource("ironOre", 10);
    bm.addResource("waterCrystal", 2);
    bm.addResource("darkCrystal", 0);
    bm.addResource("food", 3);

    // 启动
    m_view->centerOn(m_player->pos());
    m_gameRunning = true;
    m_gameOver = false;
    m_selectedBuilding = -1;
    m_moveCooldown = 0;
    m_actionCooldown = 0;
    m_monsterDamageCooldown = 0;
    m_keysHeld.clear();

    // EventManager 由其内部 timer 驱动; m_gameTimer 驱动 gameLoop
    m_gameTimer.start(16); // ~60 FPS
    m_frameClock.start();
    m_rbIdle->setChecked(true);
    SurviveData::instance().setWorkState(WorkState::Idle);
}

void MainWindow::gameLoop()
{
    if (!m_gameRunning || m_gameOver) return;

    float dt = m_frameClock.elapsed() / 1000.0f;
    m_frameClock.restart();
    if (dt > 0.1f) dt = 0.1f; // 防止跳帧

    auto &em = EventManager::instance();
    auto &sd = SurviveData::instance();

    // EventManager tick
    em.setPlayerPos(m_player->gridX(), m_player->gridY());

    // 2) 生存数值更新
    sd.update(dt, m_player->gridX(), m_player->gridY());
    if (sd.isDead()) {
        m_gameOver = true;
        m_gameRunning = false;
        m_gameTimer.stop();
        return;
    }

    // 3) 移动
    m_moveCooldown -= dt;
    float moveInterval = 0.12f / sd.moveSpeedMultiplier();
    if (m_moveCooldown <= 0 && !m_keysHeld.isEmpty()) {
        int dx = 0, dy = 0;
        if (m_keysHeld.contains(Qt::Key_W) || m_keysHeld.contains(Qt::Key_Up))    dy = -1;
        if (m_keysHeld.contains(Qt::Key_S) || m_keysHeld.contains(Qt::Key_Down))  dy =  1;
        if (m_keysHeld.contains(Qt::Key_A) || m_keysHeld.contains(Qt::Key_Left))  dx = -1;
        if (m_keysHeld.contains(Qt::Key_D) || m_keysHeld.contains(Qt::Key_Right)) dx =  1;

        if (dx != 0 || dy != 0) {
            if (m_player->tryMove(dx, dy)) {
                m_moveCooldown = moveInterval;
            }
        }
    }

    // 4) 怪物接触伤害
    m_monsterDamageCooldown -= dt;
    if (m_monsterDamageCooldown <= 0) {
        int px = m_player->gridX(), py = m_player->gridY();
        for (const auto &mon : em.cmonsters()) {
            if (mon.gridX == px && mon.gridY == py) {
                sd.addRadiation(20.0f);
                sd.addOxygen(-10.0f);
                m_monsterDamageCooldown = 1.5f;
                break;
            }
        }
    }

    // 5) View 平滑跟随
    QPointF target = m_player->pos() - QPointF(m_view->viewport()->width() / 2,
                                                m_view->viewport()->height() / 2);
    QPointF current = QPointF(m_view->horizontalScrollBar()->value(),
                               m_view->verticalScrollBar()->value());
    QPointF lerped = current + (target - current) * 0.15;
    m_view->horizontalScrollBar()->setValue(static_cast<int>(lerped.x()));
    m_view->verticalScrollBar()->setValue(static_cast<int>(lerped.y()));

    // 6) Tile 懒加载裁剪
    updateTileVisibility();

    // 7) 怪物渲染同步
    updateMonsterItems();

    // 8) 建筑渲染同步
    updateBuildingItems();

    // 9) HUD 更新
    updateHUD();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    int key = event->key();
    m_keysHeld.insert(key);

    switch (key) {
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2: case Qt::Key_3: {
        int id = key - Qt::Key_0;
        if (id < m_workGroup->buttons().size()) {
            m_workGroup->button(id)->setChecked(true);
            SurviveData::instance().setWorkState(static_cast<WorkState>(id));
        }
        break;
    }
    case Qt::Key_Space:
        switch (SurviveData::instance().workState()) {
        case WorkState::Mining:    performDig(); break;
        case WorkState::Build:     performBuild(); break;
        case WorkState::Repairing: performRepair(); break;
        default: break;
        }
        break;
    case Qt::Key_E:
        break;
    case Qt::Key_Escape:
        m_selectedBuilding = -1;
        m_rbIdle->setChecked(true);
        SurviveData::instance().setWorkState(WorkState::Idle);
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    m_keysHeld.remove(event->key());
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_player && m_view) {
        m_view->centerOn(m_player->pos());
    }
}

// =====================================================================
// Part 3: 挖掘/建造/怪物/建筑管理
// =====================================================================

void MainWindow::performDig()
{
    if (!m_player) return;
    int px = m_player->gridX(), py = m_player->gridY();
    auto &mm = MapManager::instance();

    // 挖掘相邻格 — 简化：挖掘玩家四周最近的可挖格
    const int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    for (int d = 0; d < 4; d++) {
        int tx = px + dirs[d][0], ty = py + dirs[d][1];
        Tile t = mm.getTile(tx, ty);
        if (t.durability > 0 && t.type != Tile::Bedrock && t.type != Tile::Empty) {
            auto result = mm.digTile(tx, ty, 1);
            if (result.success) {
                // 产出资源
                if (result.resourceAmount > 0) {
                    QString resKey = resourceTypeToString(result.resourceType);
                    BuildManager::instance().addResource(resKey, result.resourceAmount);
                }
                // 刷新Tile贴图
                QPoint key(tx, ty);
                if (m_tileItems.contains(key))
                    m_tileItems[key]->refreshPixmap();
                // 如果是怪物所在格，伤害怪物
                auto &em = EventManager::instance();
                auto &mons = em.monsters();
                for (int i = mons.size() - 1; i >= 0; i--) {
                    if (mons[i].gridX == tx && mons[i].gridY == ty) {
                        mons[i].hp--;
                        if (mons[i].hp <= 0) {
                            mons.removeAt(i);
                            BuildManager::instance().addResource("darkCrystal", 1);
                            QPoint mp(tx, ty);
                            if (m_monsterItems.contains(mp)) {
                                m_scene->removeItem(m_monsterItems[mp]);
                                delete m_monsterItems.take(mp);
                            }
                            emit EventManager::instance().monsterDied(mp);
                        }
                    }
                }
            }
            return; // 每次 Space 只挖一格
        }
    }
}

void MainWindow::performBuild()
{
    if (!m_player || m_selectedBuilding < 0) return;
    int px = m_player->gridX(), py = m_player->gridY();

    // 在玩家脚下放置（如果脚下是 Empty 且无建筑）
    // 或者玩家相邻格
    const int dirs[5][2] = {{0,0},{0,-1},{0,1},{-1,0},{1,0}};
    auto &bm = BuildManager::instance();
    Building::Type btype = static_cast<Building::Type>(m_selectedBuilding);

    for (int d = 0; d < 5; d++) {
        int tx = px + dirs[d][0], ty = py + dirs[d][1];
        if (bm.canPlace(tx, ty, btype)) {
            bm.placeBuilding(tx, ty, btype);
            m_selectedBuilding = -1;
            return;
        }
    }
}

void MainWindow::performRepair()
{
    // 简化：维修玩家脚下建筑（恢复其 powered 状态或只是刷新）
    // 实际上建筑不会损坏，此模式暂时等同于检查电力网格
    BuildManager::instance().updatePowerGrid();
    refreshAllBuildingVisuals();
}

QString MainWindow::resourceTypeToString(int tileType) const
{
    switch (tileType) {
    case Tile::IronOre:      return "ironOre";
    case Tile::WaterCrystal: return "waterCrystal";
    case Tile::DarkCrystal:  return "darkCrystal";
    case Tile::Crystal:      return "crystal";
    default: return "";
    }
}

void MainWindow::updateTileVisibility()
{
    QRectF viewRect(m_view->horizontalScrollBar()->value() - MapManager::TILE_SIZE * 2,
                    m_view->verticalScrollBar()->value()   - MapManager::TILE_SIZE * 2,
                    m_view->viewport()->width()  + MapManager::TILE_SIZE * 4,
                    m_view->viewport()->height() + MapManager::TILE_SIZE * 4);

    int minX = qMax(0, static_cast<int>(viewRect.left())   / MapManager::TILE_SIZE);
    int minY = qMax(0, static_cast<int>(viewRect.top())    / MapManager::TILE_SIZE);
    int maxX = qMin(MapManager::MAP_WIDTH  - 1, static_cast<int>(viewRect.right())  / MapManager::TILE_SIZE);
    int maxY = qMin(MapManager::MAP_DEPTH - 1, static_cast<int>(viewRect.bottom()) / MapManager::TILE_SIZE);

    // 移除视野外的 Item
    QList<QPoint> toRemove;
    for (auto it = m_tileItems.begin(); it != m_tileItems.end(); ++it) {
        int gx = it.key().x(), gy = it.key().y();
        if (gx < minX || gx > maxX || gy < minY || gy > maxY) {
            m_scene->removeItem(it.value());
            delete it.value();
            toRemove.append(it.key());
        }
    }
    for (const auto &k : toRemove) m_tileItems.remove(k);

    // 创建视野内新 Item
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            QPoint key(x, y);
            if (!m_tileItems.contains(key)) {
                auto *item = new TileGraphicsItem(x, y);
                m_scene->addItem(item);
                m_tileItems[key] = item;
            }
        }
    }
}

void MainWindow::updateMonsterItems()
{
    auto &em = EventManager::instance();
    const auto &mons = em.cmonsters();

    // 收集当前怪物位置
    QSet<QPoint> monPositions;
    for (const auto &m : mons) monPositions.insert(QPoint(m.gridX, m.gridY));

    // 移除已不存在的怪物的 Item
    QList<QPoint> toRemove;
    for (auto it = m_monsterItems.begin(); it != m_monsterItems.end(); ++it) {
        if (!monPositions.contains(it.key())) {
            m_scene->removeItem(it.value());
            delete it.value();
            toRemove.append(it.key());
        }
    }
    for (const auto &k : toRemove) m_monsterItems.remove(k);

    // 创建新怪物 Item 并同步位置
    for (const auto &m : mons) {
        QPoint key(m.gridX, m.gridY);
        if (m_monsterItems.contains(key)) {
            m_monsterItems[key]->setGridPos(m.gridX, m.gridY);
        } else {
            auto *item = new MonsterGraphicsItem(m.gridX, m.gridY);
            m_scene->addItem(item);
            m_monsterItems[key] = item;
        }
    }
}

void MainWindow::updateBuildingItems()
{
    auto &bm = BuildManager::instance();
    const auto buildings = bm.buildings();

    // 移除已删除建筑的 Item
    QSet<QPoint> bldKeys;
    for (auto it = buildings.begin(); it != buildings.end(); ++it)
        bldKeys.insert(it.key());

    QList<QPoint> toRemove;
    for (auto it = m_buildingItems.begin(); it != m_buildingItems.end(); ++it) {
        if (!bldKeys.contains(it.key())) {
            m_scene->removeItem(it.value());
            delete it.value();
            toRemove.append(it.key());
        }
    }
    for (const auto &k : toRemove) m_buildingItems.remove(k);

    // 创建新建筑的 Item
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        if (!m_buildingItems.contains(it.key())) {
            auto *item = new BuildingGraphicsItem(it.value());
            m_scene->addItem(item);
            m_buildingItems[it.key()] = item;
        }
    }
}

void MainWindow::refreshAllBuildingVisuals()
{
    for (auto it = m_buildingItems.begin(); it != m_buildingItems.end(); ++it)
        it.value()->refreshVisual();
}

// =====================================================================
// Part 4: HUD更新 + 存档读档 + 事件处理
// =====================================================================

void MainWindow::updateHUD()
{
    auto &bm = BuildManager::instance();
    auto &sd = SurviveData::instance();
    auto &em = EventManager::instance();
    auto &mm = MapManager::instance();

    Q_UNUSED(mm);

    // 资源
    m_lblPower->setText(QString::fromUtf8("电力: %1/%2").arg(bm.totalPower()).arg(bm.totalPower() + bm.usedPower()));
    m_lblCrystal->setText(QString::fromUtf8("水晶: %1").arg(bm.resource("crystal")));
    m_lblIronOre->setText(QString::fromUtf8("铁矿: %1").arg(bm.resource("ironOre")));
    m_lblWaterCrystal->setText(QString::fromUtf8("水结晶: %1").arg(bm.resource("waterCrystal")));
    m_lblDarkCrystal->setText(QString::fromUtf8("暗结晶: %1").arg(bm.resource("darkCrystal")));
    m_lblFood->setText(QString::fromUtf8("食物: %1").arg(bm.resource("food")));

    // 生存数值
    m_barOxygen->setValue(static_cast<int>(sd.oxygen()));
    m_barTemp->setValue(static_cast<int>(sd.bodyTemp()));
    m_barFatigue->setValue(static_cast<int>(sd.fatigue()));
    m_barRadiation->setValue(static_cast<int>(sd.radiation()));

    m_lblOxygenStatus->setText(sd.oxygenStatus());
    m_lblTempStatus->setText(sd.tempStatus());
    m_lblFatigueStatus->setText(sd.fatigueStatus());
    m_lblRadiationStatus->setText(sd.radiationStatus());
    m_lblOverallStatus->setText(sd.overallStatus());

    // 染色
    auto statusColor = [](const QString &s) -> QString {
        if (s == QString::fromUtf8("正常") || s == QString::fromUtf8("健康") || s == QString::fromUtf8("安全"))
            return "#a6e3a1";
        if (s == QString::fromUtf8("警告") || s == QString::fromUtf8("需注意"))
            return "#f9e2af";
        return "#f38ba8";
    };
    m_lblOverallStatus->setStyleSheet(QString("color: %1; font-weight: bold;").arg(statusColor(sd.overallStatus())));

    // 信息
    m_lblGameTime->setText(em.gameTime());
    if (m_player) {
        m_lblDepth->setText(QString::fromUtf8("深度: %1 层").arg(m_player->gridY()));
        int y = m_player->gridY();
        QString layer;
        if (y <= 25) layer = QString::fromUtf8("浅层岩土");
        else if (y <= 55) layer = QString::fromUtf8("结晶层");
        else layer = QString::fromUtf8("黑暗核心");
        m_lblLayer->setText(QString::fromUtf8("地层: ") + layer);
    }
}

// === 存档 ===

void MainWindow::onSaveGame()
{
    QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存游戏"),
                                                 "./saves/save.json",
                                                 "JSON (*.json)");
    if (path.isEmpty()) return;
    doSave(path);
}

void MainWindow::doSave(const QString &filepath)
{
    QJsonObject root;
    root["version"] = 1;
    root["elapsedSeconds"] = EventManager::instance().elapsedSeconds();
    root["workState"] = static_cast<int>(SurviveData::instance().workState());

    // 玩家位置
    QJsonObject playerObj;
    playerObj["x"] = m_player->gridX();
    playerObj["y"] = m_player->gridY();
    root["player"] = playerObj;

    // 地块（存所有非Empty非Bedrock的地块）
    QJsonArray tilesArr;
    auto &mm = MapManager::instance();
    for (int y = 0; y < MapManager::MAP_DEPTH; y++) {
        for (int x = 0; x < MapManager::MAP_WIDTH; x++) {
            Tile t = mm.getTile(x, y);
            if (t.type == Tile::Empty || t.type == Tile::Bedrock) {
                continue;
            }
            QJsonObject to;
            to["x"] = x; to["y"] = y;
            to["type"] = t.type;
            to["durability"] = t.durability;
            to["temperature"] = static_cast<double>(t.temperature);
            to["resource"] = t.resourceAmount;
            tilesArr.append(to);
        }
    }
    root["tiles"] = tilesArr;

    // 建筑
    QJsonArray bldArr;
    auto &bm = BuildManager::instance();
    const auto buildings = bm.buildings();
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        QJsonObject bo;
        bo["x"] = it.key().x();
        bo["y"] = it.key().y();
        bo["type"] = static_cast<int>(it.value().type);
        bldArr.append(bo);
    }
    root["buildings"] = bldArr;

    // 生存数值
    auto &sd = SurviveData::instance();
    QJsonObject survObj;
    survObj["oxygen"] = static_cast<double>(sd.oxygen());
    survObj["bodyTemp"] = static_cast<double>(sd.bodyTemp());
    survObj["fatigue"] = static_cast<double>(sd.fatigue());
    survObj["radiation"] = static_cast<double>(sd.radiation());
    root["survival"] = survObj;

    // 资源
    QJsonObject resObj;
    resObj["crystal"] = bm.resource("crystal");
    resObj["ironOre"] = bm.resource("ironOre");
    resObj["waterCrystal"] = bm.resource("waterCrystal");
    resObj["darkCrystal"] = bm.resource("darkCrystal");
    resObj["food"] = bm.resource("food");
    root["resources"] = resObj;

    QJsonDocument doc(root);
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, QString::fromUtf8("存档"),
                                 QString::fromUtf8("游戏已保存到:\n%1").arg(filepath));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             QString::fromUtf8("无法写入文件: %1").arg(filepath));
    }
}

// === 读档 ===

void MainWindow::onLoadGame()
{
    QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("读取存档"),
                                                 "./saves", "JSON (*.json)");
    if (path.isEmpty()) return;
    doLoad(path);
}

void MainWindow::doLoad(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             QString::fromUtf8("无法打开文件: %1").arg(filepath));
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("存档格式无效"));
        return;
    }

    QJsonObject root = doc.object();

    // 清除场景
    m_scene->clear();
    m_tileItems.clear();
    m_monsterItems.clear();
    m_buildingItems.clear();
    m_player = nullptr;

    // 停止当前游戏循环
    m_gameTimer.stop();
    m_gameRunning = false;

    // 重建地图（先生成默认地图，再覆写存档中的地块数据）
    MapManager::instance().generateMap(0);
    QJsonArray tilesArr = root["tiles"].toArray();
    auto &mm = MapManager::instance();
    for (const auto &val : tilesArr) {
        QJsonObject to = val.toObject();
        int x = to["x"].toInt(), y = to["y"].toInt();
        Tile t;
        t.type = to["type"].toInt();
        t.durability = to["durability"].toInt();
        t.temperature = static_cast<float>(to["temperature"].toDouble());
        t.resourceAmount = to["resource"].toInt();
        mm.setTile(x, y, t);
    }

    // 清空建造管理器
    auto &bm = BuildManager::instance();
    {
        auto keys = bm.buildings().keys();
        for (const auto &k : keys) bm.removeBuilding(k.x(), k.y());
    }

    // 恢复建筑（使用 restoreBuilding 直接插入）
    QJsonArray bldArr = root["buildings"].toArray();
    for (const auto &val : bldArr) {
        QJsonObject bo = val.toObject();
        int x = bo["x"].toInt(), y = bo["y"].toInt();
        Building::Type btype = static_cast<Building::Type>(bo["type"].toInt());
        Building b;
        b.type = btype;
        b.gridX = x;
        b.gridY = y;
        Building::buildingSize(btype, b.width, b.height);
        b.powered = false;
        b.active = true;
        bm.restoreBuilding(b);
    }
    bm.updatePowerGrid();

    // 恢复资源（先清空再添加）
    bm.clearResources();
    QJsonObject resObj = root["resources"].toObject();
    bm.addResource("crystal",     resObj["crystal"].toInt());
    bm.addResource("ironOre",     resObj["ironOre"].toInt());
    bm.addResource("waterCrystal",resObj["waterCrystal"].toInt());
    bm.addResource("darkCrystal", resObj["darkCrystal"].toInt());
    bm.addResource("food",        resObj["food"].toInt());

    // 恢复生存数值
    QJsonObject survObj = root["survival"].toObject();
    SurviveData::instance().reset();
    SurviveData::instance().restore(
        static_cast<float>(survObj["oxygen"].toDouble()),
        static_cast<float>(survObj["bodyTemp"].toDouble()),
        static_cast<float>(survObj["fatigue"].toDouble()),
        static_cast<float>(survObj["radiation"].toDouble())
    );

    // 创建玩家
    QJsonObject playerObj = root["player"].toObject();
    int px = playerObj["x"].toInt(), py = playerObj["y"].toInt();
    m_player = new PlayerGraphicsItem;
    m_player->setGridPos(px, py);
    m_scene->addItem(m_player);

    // 恢复工作状态
    int ws = root["workState"].toInt();
    SurviveData::instance().setWorkState(static_cast<WorkState>(ws));
    if (ws < m_workGroup->buttons().size())
        m_workGroup->button(ws)->setChecked(true);

    // 恢复时间
    EventManager::instance().reset();
    EventManager::instance().setElapsedSeconds(root["elapsedSeconds"].toInt());
    EventManager::instance().setPlayerPos(px, py);

    // 启动
    updateTileVisibility();
    updateBuildingItems();
    m_view->centerOn(m_player->pos());
    m_gameRunning = true;
    m_gameOver = false;
    m_selectedBuilding = -1;
    m_moveCooldown = 0;
    m_actionCooldown = 0;
    m_monsterDamageCooldown = 0;
    m_gameTimer.start(16);
    m_frameClock.start();
    m_rbIdle->setChecked(true);
}

void MainWindow::onNewGame()  { startNewGame(); }

void MainWindow::onToggleFullscreen()
{
    if (isFullScreen()) showNormal(); else showFullScreen();
}

void MainWindow::onBuildButtonClicked(int id)
{
    m_selectedBuilding = id;
    m_rbBuilding->setChecked(true);
    SurviveData::instance().setWorkState(WorkState::Build);
}

void MainWindow::onCancelBuild()
{
    m_selectedBuilding = -1;
    m_rbIdle->setChecked(true);
    SurviveData::instance().setWorkState(WorkState::Idle);
}

void MainWindow::onMonsterSpawned(QPoint pos)
{
    // MonsterItem 由 updateMonsterItems 在 gameLoop 中自动创建
    Q_UNUSED(pos);
}

void MainWindow::onMonsterDied(QPoint pos)
{
    Q_UNUSED(pos);
}

void MainWindow::onDeath(const QString &reason)
{
    m_gameOver = true;
    m_gameRunning = false;
    m_gameTimer.stop();
    QMessageBox::information(this, QString::fromUtf8("游戏结束"),
                             QString::fromUtf8("你死了。\n原因: %1").arg(reason));
}

void MainWindow::onBuildingPlaced(QPoint pos)
{
    // 由 updateBuildingItems 处理
    Q_UNUSED(pos);
    refreshAllBuildingVisuals();
}

void MainWindow::onBuildingRemoved(QPoint pos)
{
    Q_UNUSED(pos);
    refreshAllBuildingVisuals();
}
