#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "memoryview.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct Segment {
    QString name; //is it code OR data OR stack
    int size; //size of allocation
    int baseAddress; //starting address
    bool allocated; //flag to indicate whther process in memory
};

struct Process {
    int id;
    QString name; //p1 for ex
    QVector <Segment> segments;
    bool fullyAllocated; //flag to indicate whether all segments allocated
};

struct Hole {
    int size;
    int startAddress;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    //buttons logic
    void on_btn_setMemory_clicked();
    void on_btn_addHole_clicked();
    void on_btn_addSegment_clicked();
    void on_btn_allocate_clicked();
    void on_btn_deallocate_clicked();
    void on_btn_reset_clicked();

private:
    Ui::MainWindow *ui;

    MemoryView * memoryView;
    int totalMemory = 0;
    int processCnt = 0;
    QVector <Process> processes;
    QVector <Segment> pendingSegments; //segments built before allocation
    QVector <Hole> holes;

    void allocateProcess(Process& p);
    void deallocateProcess(int processId);
    void mergeHoles();
    int  firstFit(int segmentSize); //return hole idx or -1
    int  bestFit(int segmentSize); //return hole idx or -1

    //called after each allocation/ deallocation
    void updateMemoryView(); //redraw memory layout
    void updateHolesTable(); //refresh holes table
    void updateSegmentTable(int pid); //show segment table for a process

};
#endif // MAINWINDOW_H
