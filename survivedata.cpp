#include "survivedata.h"
#include "mapmanager.h"
#include "buildmanager.h"
#include <QtMath>

SurviveData& SurviveData::instance()
{
    static SurviveData inst;
    return inst;
}

void SurviveData::reset()
{
    m_oxygen = 100;
    m_bodyTemp = 50;
    m_fatigue = 0;
    m_radiation = 0;
    m_lowOxygenTimer = 0;
    m_workState = WorkState::Idle;
    m_dead = false;
    m_deathReason.clear();
}

void SurviveData::update(float dt, int px, int py)
{
    if (m_dead) return;

    auto &mm = MapManager::instance();
    auto &bm = BuildManager::instance();
    Tile tile = mm.getTile(px, py);

    // 1) 氧气: 自然-0.5/秒，制氧机半径6格内恢复
    float o2delta = -0.5f;
    auto nearby = bm.getBuildingsInRadius(px, py, 6);
    for (const auto *b : nearby) {
        if (b->type == Building::OxygenMaker && b->powered) {
            o2delta += 5.0f;
        }
    }
    m_oxygen += o2delta * dt;

    // 氧气耗尽计时
    if (m_oxygen <= 0) {
        m_oxygen = 0;
        m_lowOxygenTimer += dt;
        if (m_lowOxygenTimer >= 3.0f) {
            m_dead = true;
            m_deathReason = QString::fromUtf8("缺氧窒息死亡");
            emit death(m_deathReason);
            return;
        }
    } else {
        m_lowOxygenTimer = qMax(0.0f, m_lowOxygenTimer - dt * 2.0f);
    }
    m_oxygen = qBound(0.0f, m_oxygen, 100.0f);

    // 2) 体温: 向地块温度靠拢
    float tempTarget = tile.temperature;
    // 发电机附近温暖
    for (const auto *b : nearby) {
        if (b->type == Building::Generator && b->powered)
            tempTarget = qMax(tempTarget, 25.0f);
    }
    float tempDiff = tempTarget - m_bodyTemp;
    m_bodyTemp += tempDiff * 0.3f * dt;
    m_bodyTemp = qBound(0.0f, m_bodyTemp, 100.0f);
    if (m_bodyTemp <= 10.0f) {
        m_dead = true;
        m_deathReason = QString::fromUtf8("体温过低死亡");
        emit death(m_deathReason);
        return;
    }
    if (m_bodyTemp >= 90.0f) {
        m_dead = true;
        m_deathReason = QString::fromUtf8("体温过高死亡");
        emit death(m_deathReason);
        return;
    }

    // 3) 疲劳: 自然累积+工作倍率
    float fatigueRate = 0.2f;
    switch (m_workState) {
    case WorkState::Mining:    fatigueRate *= 1.5f; break;
    case WorkState::Build:     fatigueRate *= 1.2f; break;
    case WorkState::Repairing: fatigueRate *= 1.3f; break;
    default: break;
    }
    // 种植舱恢复
    for (const auto *b : nearby) {
        if (b->type == Building::GrowPod && b->powered)
            fatigueRate -= 0.15f;
    }
    m_fatigue += fatigueRate * dt;
    m_fatigue = qBound(0.0f, m_fatigue, 100.0f);

    // 4) 辐射: 深层递增
    float radRate = 0.0f;
    if (py > 30) radRate = 0.05f;
    if (py > 50) radRate = 0.12f;
    if (py > 60) radRate = 0.2f;
    // 减压器半径5格内降低
    for (const auto *b : nearby) {
        if (b->type == Building::Decompressor && b->powered)
            radRate -= 1.5f;
    }
    m_radiation += radRate * dt;
    m_radiation = qBound(0.0f, m_radiation, 100.0f);

    // 辐射≥80 → 扣氧气
    if (m_radiation >= 80.0f) {
        m_oxygen -= 2.0f * dt;
    }

    emit statusChanged();
}

bool SurviveData::isDead() const { return m_dead; }
QString SurviveData::deathReason() const { return m_deathReason; }

float SurviveData::moveSpeedMultiplier() const
{
    if (m_fatigue >= 100.0f) return 0.5f;
    return 1.0f;
}

void SurviveData::addRadiation(float amount)
{
    m_radiation = qBound(0.0f, m_radiation + amount, 100.0f);
    emit statusChanged();
}

void SurviveData::addOxygen(float amount)
{
    m_oxygen = qBound(0.0f, m_oxygen + amount, 100.0f);
    emit statusChanged();
}

void SurviveData::restore(float ox, float temp, float fat, float rad)
{
    m_oxygen = qBound(0.0f, ox, 100.0f);
    m_bodyTemp = qBound(0.0f, temp, 100.0f);
    m_fatigue = qBound(0.0f, fat, 100.0f);
    m_radiation = qBound(0.0f, rad, 100.0f);
    emit statusChanged();
}

QString SurviveData::levelText(float v)
{
    if (v >= 80) return QString::fromUtf8("正常");
    if (v >= 50) return QString::fromUtf8("警告");
    if (v >= 20) return QString::fromUtf8("危险");
    return QString::fromUtf8("濒死");
}

QString SurviveData::oxygenStatus() const    { return levelText(m_oxygen); }
QString SurviveData::tempStatus() const
{
    if (m_bodyTemp >= 15 && m_bodyTemp <= 85) return QString::fromUtf8("正常");
    if (m_bodyTemp >= 10 && m_bodyTemp <= 90) return QString::fromUtf8("警告");
    return QString::fromUtf8("危险");
}
QString SurviveData::fatigueStatus() const
{
    if (m_fatigue < 50) return QString::fromUtf8("正常");
    if (m_fatigue < 80) return QString::fromUtf8("疲劳");
    return QString::fromUtf8("过劳");
}
QString SurviveData::radiationStatus() const
{
    if (m_radiation < 30) return QString::fromUtf8("安全");
    if (m_radiation < 60) return QString::fromUtf8("警告");
    if (m_radiation < 80) return QString::fromUtf8("危险");
    return QString::fromUtf8("辐射病");
}

QString SurviveData::overallStatus() const
{
    if (isDead()) return QString::fromUtf8("死亡");
    float worst = qMin(qMin(m_oxygen, 100 - m_fatigue),
                       qMin(100.0f - m_radiation, m_bodyTemp));
    if (worst > 60) return QString::fromUtf8("健康");
    if (worst > 30) return QString::fromUtf8("需注意");
    return QString::fromUtf8("危急");
}
