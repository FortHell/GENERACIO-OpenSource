#include "HMDTile.h"

HMDTile::HMDTile(const QString& name, QWidget* parent)
    : QPushButton(parent), m_name(name)
{
    setFixedSize(260, 160);
    setStyleSheet(R"(
        QPushButton { 
            background-color: #2a2a2a; 
            border-radius: 12px; 
        }
        QPushButton:hover { 
            background-color: #333333; 
        }
        QPushButton:pressed { 
            background-color: #2b2b2b; 
        }
        QLabel { 
            background-color: transparent; 
            color: white; 
            font-size: 38px; 
            font-family: Arial, sans-serif; 
        }
    )");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    QLabel* title = new QLabel(name);
    title->setAlignment(Qt::AlignCenter);

    layout->addStretch();
    layout->addWidget(title);
    layout->addStretch();
}

void HMDTile::mousePressEvent(QMouseEvent* event) {
    QPushButton::mousePressEvent(event);
    emit clicked(m_name);
}