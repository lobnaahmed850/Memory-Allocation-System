#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <climits>
#include <QMessageBox>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    memoryView = new MemoryView(this);
    ui->scrollArea_memory->setWidget(memoryView);
    ui->scrollArea_memory->setWidgetResizable(true);

    ui->tableWidget_holes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_segmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tableWidget_holes->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_segmentTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->comboBox_selectProcess,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                QString name = ui->comboBox_selectProcess->currentText();
                for (auto& p : processes)
                    if (p.name == name) { updateSegmentTable(p.id); return; }
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::allocateProcess(Process & p) {
    QVector <Hole> tempHoles = holes;
    QVector <QPair<int,int>> placements; //store result for each segment : hole idx, baseAdd
    bool allFit=true;

    for(int i=0;i<p.segments.size();i++) {
        int segSize = p.segments[i].size;
        int chosenIdx = -1;

        if(ui->rb_firstFit->isChecked()) {
            for(int j=0;j<tempHoles.size();j++) {
                if(tempHoles[j].size >= segSize) {
                    chosenIdx=j; break;
                }
            }
        }
        else { //bestFit chosen
            int bestSize=INT_MAX;
            for(int j=0;j<tempHoles.size();j++) {
                if(tempHoles[j].size>=segSize && tempHoles[j].size < bestSize) {
                    bestSize = tempHoles[j].size;
                    chosenIdx = j;
                }
            }
        }
        if(chosenIdx==-1) { //if segment doesn't fit, reject process
            allFit=false;
            break;
        }
        placements.append({chosenIdx, tempHoles[chosenIdx].startAddress}); //record segment
        tempHoles[chosenIdx].startAddress += segSize;
        tempHoles[chosenIdx].size -= segSize;

        if(tempHoles[chosenIdx].size==0) tempHoles.remove(chosenIdx);
    }

        if(!allFit) { //reject if any segment doesn't fit
        QMessageBox::warning (this, "Allocation Failed", "Process " + p.name + " can't be allocated.\n");
            p.fullyAllocated=false;
        pendingSegments.clear();
            ui->listWidget_pendingSegments->clear();
        return;
        }
        // if all fits -> place into real holes
        holes = tempHoles;
        for(int i=0;i<p.segments.size();i++) {
            p.segments[i].baseAddress=placements[i].second;
            p.segments[i].allocated=true;
        }
        p.fullyAllocated=true;
        processes.append(p);
        QMessageBox::information (this, "Success", "Process " + p.name + " allocated successfully. :)");
}

void MainWindow::mergeHoles() {
    //sort first so adjacent check works well
    std::sort(holes.begin(),holes.end(), [] (const Hole & a, const Hole & b) {
return a.startAddress<b.startAddress;
    });

    int i=0;
    while(i<holes.size()-1) {
        int currentEnd = holes[i].startAddress + holes[i].size;
        if(currentEnd==holes[i+1].startAddress) { //merge i+1 into i
            holes[i].size+=holes[i+1].size;
            holes.remove(i+1);
        }
        else i++;
    }
}

void MainWindow::deallocateProcess(int processId) {
    //find process first
    int processIdx=-1;
    for(int i=0;i<processes.size();i++) {
        if(processId==processes[i].id) {
            processIdx=i;
            break;
        }
    }
    if (processIdx==-1) {
        QMessageBox::warning(this, "Error", "Process not found.");
        return;
    }
    Process & p = processes[processIdx];

    //convert each segment into hole
    for(int i=0;i<p.segments.size();i++) {
        Segment & seg = p.segments[i];
        if(seg.allocated) {
            Hole newHole;
            newHole.startAddress=seg.baseAddress;
            newHole.size=seg.size;
            holes.append(newHole);
        }
    }

    //remove process
    QString name = p.name;
    processes.remove(processIdx);

    //merge any adjacent holes
    mergeHoles();

    QMessageBox::information(this,"Success","Process " + name+" deallocated successfully. :)");
}

void MainWindow::on_btn_deallocate_clicked() {
    QString selectedName = ui->comboBox_selectProcess->currentText();

    if (selectedName.isEmpty()) {
        QMessageBox::warning(this, "Error", "No process selected.");
        return;
    }

    int pid=-1;
    for(auto & p: processes) {
        if(selectedName==p.name) {
            pid=p.id; break;
        }
    }
    if(pid==-1) {
        QMessageBox::warning(this, "Error", "Process not found.");
        return;
    }
    deallocateProcess(pid);
    int idx = ui->comboBox_selectProcess->findText(selectedName);
    if (idx != -1)
        ui->comboBox_selectProcess->removeItem(idx);

    updateHolesTable();
    updateMemoryView();
    ui->tableWidget_segmentTable->setRowCount(0);
}

void::MainWindow ::updateMemoryView() {
    QVector <MemoryBlock> blocks;
    //add allocated segments
    for(const auto & p:processes) {
        for(const auto & seg: p.segments) {
            if(seg.allocated) {
                MemoryBlock b;
                b.startAddress = seg.baseAddress;
                b.size = seg.size;
                b.label = p.name + "\n" + seg.name;
                b.isHole = false;
                b.pid = p.id;
                blocks.append(b);
            }
        }
    }
    for(const auto & h : holes) {
        MemoryBlock b;
        b.startAddress = h.startAddress;
        b.isHole = true;
        b.label = "HOLE\n" + QString::number(h.size) + "KB";
        b.size = h.size;
        b.pid = 0;
        blocks.append(b);
    }
    std::sort(blocks.begin(),blocks.end(),[](const MemoryBlock & a, const MemoryBlock & b) {
        return a.startAddress < b.startAddress;
    });
    memoryView->setTotalMemory(totalMemory);
    memoryView->setBlocks(blocks);
}

void MainWindow::on_btn_setMemory_clicked() {
    int mem = ui->spinBox_totalMemory->value();
    if (mem <= 0) {
        QMessageBox::warning(this, "Error", "Memory size must be > 0.");
        return;
    }
    totalMemory = mem;
    memoryView->setTotalMemory(totalMemory);
    memoryView->update();
    QMessageBox::information(this, "Memory Set",
                             "Total memory set to " + QString::number(totalMemory) + " KB.");
}
void MainWindow::on_btn_addHole_clicked() {
    if (totalMemory == 0) {
        QMessageBox::warning(this, "Error", "Set total memory first.");
        return;
    }

    int start = ui->spinBox_holeStart->value();
    int size  = ui->spinBox_holeSize->value();

    // validate hole fits within memory
    if (start + size > totalMemory) {
        QMessageBox::warning(this, "Error",
                             "Hole exceeds total memory size.");
        return;
    }

    // validate no overlap with existing holes
    for (auto& h : holes) {
        int hEnd = h.startAddress + h.size;
        int newEnd = start + size;
        if (start < hEnd && newEnd > h.startAddress) {
            QMessageBox::warning(this, "Error",
                                 "Hole overlaps with an existing hole.");
            return;
        }
    }
    Hole h;
    h.startAddress = start;
    h.size         = size;
    holes.append(h);

    mergeHoles(); // merge if adjacent to existing holes
    updateHolesTable();
    updateMemoryView();
}

void MainWindow::on_btn_addSegment_clicked() {
    QString segName = ui->lineEdit_segName->text().trimmed();
    int     segSize = ui->spinBox_segSize->value();

    if (segName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Segment name cannot be empty.");
        return;
    }
    if (segSize <= 0) {
        QMessageBox::warning(this, "Error", "Segment size must be > 0.");
        return;
    }

    // add to pending list
    Segment seg;
    seg.name        = segName;
    seg.size        = segSize;
    seg.baseAddress = -1;
    seg.allocated   = false;
    pendingSegments.append(seg);
    // show in listWidget
    ui->listWidget_pendingSegments->addItem(
        segName + " — " + QString::number(segSize) + " KB"
        );

    // clear inputs
    ui->lineEdit_segName->clear();
    ui->spinBox_segSize->setValue(1);
}

void MainWindow::on_btn_allocate_clicked() {
    if (totalMemory == 0) {
        QMessageBox::warning(this, "Error", "Set total memory first.");
        return;
    }

    QString procName = ui->lineEdit_processName->text().trimmed();
    if (procName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Process name cannot be empty.");
        return;
    }

    // check duplicate name
    for (auto& p : processes) {
        if (p.name == procName) {
            QMessageBox::warning(this, "Error",
                                 "Process " + procName + " already exists.");
            return;
        }
    }

    if (pendingSegments.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             "Add at least one segment before allocating.");
        return;
    }

    if (holes.isEmpty()) {
        QMessageBox::warning(this, "Error",
                             "No free holes available in memory.");
        return;
    }
    // build process
    Process p;
    p.id             = ++processCnt;
    p.name           = procName;
    p.segments       = pendingSegments;
    p.fullyAllocated = false;

    // try to allocate
    allocateProcess(p);

    if (p.fullyAllocated) {
        // add to process dropdown
        ui->comboBox_selectProcess->addItem(p.name);

        // clear pending segments
        pendingSegments.clear();
        ui->listWidget_pendingSegments->clear();
        ui->lineEdit_processName->clear();

        // refresh all UI
        updateHolesTable();
        updateMemoryView();
        updateSegmentTable(p.id);
    }
}

void MainWindow::on_btn_reset_clicked() {
    processes.clear();
    holes.clear();
    pendingSegments.clear();
    totalMemory    = 0;
    processCnt = 0;

    ui->listWidget_pendingSegments->clear();
    ui->comboBox_selectProcess->clear();
    ui->tableWidget_holes->setRowCount(0);
    ui->tableWidget_segmentTable->setRowCount(0);
    ui->lineEdit_processName->clear();
    ui->lineEdit_segName->clear();

    memoryView->clear();
}

void MainWindow::updateHolesTable() {
    ui->tableWidget_holes->setRowCount(holes.size());
    for (int i = 0; i < holes.size(); i++) {
        ui->tableWidget_holes->setItem(i, 0,
                                       new QTableWidgetItem(QString::number(holes[i].startAddress)));
        ui->tableWidget_holes->setItem(i, 1,
                                       new QTableWidgetItem(QString::number(holes[i].size)));
    }
}

void MainWindow::updateSegmentTable(int pid) {
    ui->tableWidget_segmentTable->setRowCount(0);
    for (auto& p : processes) {
        if (p.id == pid) {
            for (auto& seg : p.segments) {
                int row = ui->tableWidget_segmentTable->rowCount();
                ui->tableWidget_segmentTable->insertRow(row);
                ui->tableWidget_segmentTable->setItem(row, 0,
                                                      new QTableWidgetItem(seg.name));
                ui->tableWidget_segmentTable->setItem(row, 1,
                                                      new QTableWidgetItem(QString::number(seg.baseAddress)));
                ui->tableWidget_segmentTable->setItem(row, 2,
                                                      new QTableWidgetItem(QString::number(seg.size)));
            }
            return;
        }
    }
}

















