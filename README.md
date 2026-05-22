# Memory Segmentation Simulator

An interactive desktop application built using **C++** and the **Qt Framework** that simulates the segmentation memory management scheme. The simulator models how operating systems allocate and deallocate processes based on a user's view of a program (broken into logical segments like Code, Data, and Stack), providing live visual mapping and structural comparison between allocation strategies.

---

## 📊 Features & Core Capabilities
* **Dual Allocation Engine:** Implements and compares **First-Fit** and **Best-Fit** placement algorithms.
* **Atomic Two-Phase Allocation:** Validates the entire process entry list on a memory buffer before altering real memory states to guarantee an all-or-nothing configuration.
* **Defragmentation via Hole Merging:** Automatically merges contiguous free fragments into a single larger block upon process removal.
* **Dynamic Graphing:** Leverages Qt's `QPainter` to draw a proportionally scaled, HSV-colored memory model layout with anti-overlapping address guidelines.

---

## System Architecture & Data Structures

The system handles memory configuration using four structures declared inside `mainwindow.h` and `memoryview.h`:

```cpp
// 1. Represents an individual process segment
struct Segment {
    QString name;        // Segment identifier (e.g., Code, Data, Stack)
    int size;            // Requested capacity in KB
    int baseAddress;     // Runtime starting boundary offset (-1 if unallocated)
    bool allocated;      // Allocation state indicator
};

// 2. Models a comprehensive process container
struct Process {
    int id;                   // Unique integer identifier
    QString name;             // Functional process name (e.g., P1)
    QVector<Segment> segments; // Internal sequential segment registry
    bool fullyAllocated;      // Confirms if all child segments successfully fit
};

// 3. Identifies an unallocated memory partition
struct Hole {
    int startAddress;         // Segment base offset point
    int size;                 // Fragment capacity in KB
};

// 4. Utilized by the UI component for rendering blocks
struct MemoryBlock {
    int startAddress;         // Relative pixel base start address
    int size;                 // Scaled block length
    QString label;            // Display text drawn inside the partition
    bool isHole;              // Color flag (Gray if a Hole, colored if an active Segment)
    int pid;                  // Parent process ID mapping for distinct color selection
};
