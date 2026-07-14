#ifndef SURVIVEDATA_H
#define SURVIVEDATA_H

#include <QObject>
#include <QString>

enum class WorkState { Idle = 0, Mining, Build, Repairing };

class SurviveData : public QObject
{
    Q_OBJECT
public:
    static SurviveData& instance();

    void reset();

    // dt=秒, px/py=玩家格坐标
    void update(float dt, int px, int py);

    bool isDead() const;
    QString deathReason() const;

    // Getters
    float oxygen() const       { return m_oxygen; }
    float bodyTemp() const     { return m_bodyTemp; }
    float fatigue() const      { return m_fatigue; }
    float radiation() const    { return m_radiation; }
    WorkState workState() const { return m_workState; }
    void setWorkState(WorkState s) { m_workState = s; }

    float moveSpeedMultiplier() const;

    // 直接增减（用于Monster接触伤害等）
    void addRadiation(float amount);
    void addOxygen(float amount);

    // 恢复存档值
    void restore(float ox, float temp, float fat, float rad);

    // 状态文字
    QString oxygenStatus() const;
    QString tempStatus() const;
    QString fatigueStatus() const;
    QString radiationStatus() const;
    QString overallStatus() const;

signals:
    void death(const QString &reason);
    void statusChanged();

private:
    SurviveData() = default;
    SurviveData(const SurviveData&) = delete;
    SurviveData& operator=(const SurviveData&) = delete;

    static QString levelText(float v);

    float m_oxygen = 100.0f;
    float m_bodyTemp = 50.0f;
    float m_fatigue = 0.0f;
    float m_radiation = 0.0f;
    float m_lowOxygenTimer = 0.0f;
    WorkState m_workState = WorkState::Idle;
    bool m_dead = false;
    QString m_deathReason;
};

#endif // SURVIVEDATA_H
