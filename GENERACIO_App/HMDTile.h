#pragma once
#include <QtWidgets>

class HMDTile : public QPushButton {
    Q_OBJECT
public:
    HMDTile(const QString& name, QWidget* parent = nullptr);

signals:
    void clicked(const QString& name);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString m_name;
};