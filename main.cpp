#include "DAG.h"
#include <iostream>

struct System {
    std::string name;
};

int main() {
    // 1. Initialize Pool and DAG
    ThreadPool pool(4);
    DAG<System> dag;

    // 2. Add Nodes
    auto src = dag.add_node({"InputReader"});
    auto wA  = dag.add_node({"WorkerA"});
    auto wB  = dag.add_node({"WorkerB"});
    auto agg = dag.add_node({"Aggregator"});

    // 3. Define Dependencies
    dag.add_edge(src, wA);
    dag.add_edge(src, wB);
    dag.add_edge(wA, agg);
    dag.add_edge(wB, agg);

    // 4. Execute
    dag.execute_parallel(pool, [&](NodeID id, System& sys) {
        if (sys.name == "InputReader") {
            dag.set_port_value(id, 100); // Pass data to successors
        } 
        else if (sys.name == "WorkerA") {
            int val = dag.get_port_value<int>(src);
            dag.set_port_value(id, val + 50);
        }
        else if (sys.name == "WorkerB") {
            int val = dag.get_port_value<int>(src);
            dag.set_port_value(id, val * 2);
        }
        else if (sys.name == "Aggregator") {
            int resA = dag.get_port_value<int>(wA);
            int resB = dag.get_port_value<int>(wB);
            std::cout << "Result: " << (resA + resB) << std::endl;
        }
    });

    return 0;
}