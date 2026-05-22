#ifndef MEMORYVIEW_H
#define MEMORYVIEW_H

#include <QWidget>
#include <QVector>
#include <QString>

struct MemoryBlock {
    int startAddress;
    int size;
    QString label;
    bool isHole;
    int pid;
};

class MemoryView : public QWidget {
    Q_OBJECT;

public:
    explicit MemoryView(QWidget * parent=nullptr);

    void setTotalMemory(int total);
    void setBlocks(const QVector<MemoryBlock> &b);
    void clear();
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    int totalMemory=0;
    QVector<MemoryBlock> blocks;
};

#endif // MEMORYVIEW_H
