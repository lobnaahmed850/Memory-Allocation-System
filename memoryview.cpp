#include "memoryview.h"
#include <QPainter>
#include <QFont>

MemoryView::MemoryView(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(250);
}

void MemoryView::setTotalMemory(int total) {
    totalMemory = total;
}

void MemoryView::setBlocks(const QVector<MemoryBlock>& b) {
    blocks = b;
    update(); // trigger repaint
}

void MemoryView::clear() {
    blocks.clear();
    totalMemory = 0;
    update();
}

void MemoryView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (totalMemory == 0) {
        painter.drawText(rect(), Qt::AlignCenter, "Set total memory to begin.");
        return;
    }

    int barY = 20;
    int barH = 70;
    int barW = width() - 40;
    int barX = 20;

    //draw all blocks
    for (const auto& block : blocks) {
        int xStart = barX + (int)((float)block.startAddress / totalMemory * barW);
        int xEnd   = barX + (int)((float)(block.startAddress + block.size) / totalMemory * barW);
        int w      = xEnd - xStart;
        if (w < 1) w = 1;

        QColor color;
        if (block.isHole)
            color = QColor(200, 200, 200);
        else
            color = QColor::fromHsv((block.pid * 60) % 360, 160, 220);

        painter.setBrush(color);
        painter.setPen(Qt::black);
        painter.drawRect(xStart, barY, w, barH);

        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 9));
        painter.drawText(
            QRect(xStart, barY, w, barH),
            Qt::AlignCenter | Qt::TextWordWrap,
            block.label
            );
    }

    //use QMap so addresses are sorted and deduplicated automatically
    QMap<int, int> addressToX;

    for (const auto& block : blocks) {
        int xStart = barX + (int)((float)block.startAddress / totalMemory * barW);
        int xEnd   = barX + (int)((float)(block.startAddress + block.size) / totalMemory * barW);
        addressToX[block.startAddress]              = xStart;
        addressToX[block.startAddress + block.size] = xEnd;
    }
    //always include 0 and totalMemory
    addressToX[0]           = barX;
    addressToX[totalMemory] = barX + barW;

    //draw labels, skip if too close to previous
    int minSpacing  = 35;
    int lastDrawnX  = -minSpacing;

    painter.setPen(Qt::darkGray);
    painter.setFont(QFont("Arial", 8));

    for (auto it = addressToX.begin(); it != addressToX.end(); ++it) {
        int address = it.key();
        int x       = it.value();

        if (x - lastDrawnX >= minSpacing) {
            //draw tick line
            painter.drawLine(x, barY + barH, x, barY + barH + 5);
            //draw address text
            painter.drawText(x - 10, barY + barH + 18, QString::number(address));
            lastDrawnX = x;
        }
    }

    //outer border
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(barX, barY, barW, barH);
}