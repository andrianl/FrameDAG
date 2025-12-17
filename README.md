# FrameDAG

A high-performance, template-based C++ task graph executor designed for concurrent game engine systems. **FrameDAG** allows you to define complex system dependencies and execute them in parallel, maximizing CPU utilization during each game frame.

---

## Features

* **Header-only:** Zero dependencies, just drop `DAG.h` into your project.
* **Lock-free Path:** Uses `std::atomic` counters for dependency resolution during execution.
* **Type-safe Ports:** Share data between nodes using `std::any` with a simple `set/get` API.
* **Game-Ready:** Optimized `reset()` mechanism allows re-running the same graph structure every frame without re-allocations.
* **Lightweight ThreadPool:** Includes a minimalist task-based worker pool.

---

## Installation

Since **FrameDAG** is a header-only library:

1. Copy `DAG.h` into your project's include directory.
2. Ensure your compiler is set to **C++17** or higher.

---

## Development & Testing (Premake)

The repository includes a test suite (`main.cpp`) and **Premake5** scripts to quickly generate project files for your preferred IDE or compiler.

### How to Generate the Test Project:

1. **Download** the [Premake5 binary](https://premake.github.io/download/) and place it in the root folder.
2. **Run the script** for your OS:
* **Windows:** Run `GenerateProjectFiles.bat` (Generates a Visual Studio 2022 solution).
* **Linux/macOS:** Run `sh GenerateProjectFiles.sh` (Generates a GNU Makefile).


3. **Build & Run:**
* On Windows, open the `.sln` file.
* On Linux/macOS, type `make` in the terminal and run the binary from the `bin/` folder.



---

## API Reference

### `DAG<T>`

* **`NodeID add_node(T data)`**: Adds a system to the graph. Returns a unique ID.
* **`void add_edge(NodeID from, NodeID to)`**: `to` will wait for `from` to finish.
* **`void execute_parallel(Pool& pool, Func executor)`**: Runs the graph. `executor` is `void(NodeID, T&)`.
* **`void reset()`**: Fast reset of atomic counters for the next frame.
* **`void set_port_value(NodeID id, std::any value)`**: Write to a node's output.
* **`T get_port_value<T>(NodeID id)`**: Read from a node's output.

---

## Quick Start

```cpp
#include "DAG.h"
#include <iostream>

int main() {
    ThreadPool pool(4);
    DAG<std::string> dag;

    auto p = dag.add_node("Physics");
    auto a = dag.add_node("AI");
    dag.add_edge(p, a); // AI depends on Physics

    dag.execute_parallel(pool, [](NodeID id, std::string& data) {
        std::cout << "Executing: " << data << std::endl;
    });

    return 0;
}

```

---

## Dependency Visualization

FrameDAG excels at handling non-linear dependencies like the **Diamond Pattern**:

```text
       [ Source ]
        /      \
    [Work A] [Work B]
        \      /
      [Aggregator]

```

---