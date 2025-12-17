#pragma once 

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <any>
#include <memory>

/**
 * @class ThreadPool
 * @brief A simple fixed-size thread pool for executing arbitrary tasks.
 */
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;

public:
    /**
     * @brief Construct a new Thread Pool.
     * @param threads Number of worker threads. Defaults to hardware concurrency.
     */
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    /**
     * @brief Enqueue a task for asynchronous execution.
     * @param f A functional object (lambda, function pointer, etc.)
     */
    void enqueue(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(std::move(f));
        }
        cv.notify_one();
    }

    /**
     * @brief Joins all threads and stops the pool.
     */
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
};

/**
 * @typedef NodeID
 * @brief Unique identifier for a node based on its insertion index.
 */
using NodeID = size_t;

/**
 * @class DAG
 * @brief A Template Directed Acyclic Graph execution engine.
 * @tparam T The type of data stored in each node.
 */
template <typename T>
class DAG {
    struct Node {
        T data;
        std::any output; // Storage for inter-node communication (Ports)
    };

    std::vector<Node> nodes;
    std::vector<std::vector<NodeID>> adj; // Successors list
    std::vector<int> base_indegree;       // Initial dependency counts

    // Runtime state variables
    std::vector<std::atomic<int>> current_indegree;
    std::atomic<size_t> remaining;

public:
    /**
     * @brief Add a node to the graph.
     * @param data The user data/system to be stored.
     * @return NodeID The unique ID assigned to this node.
     */
    NodeID add_node(T data) {
        NodeID id = nodes.size();
        nodes.push_back({ std::move(data), std::any() });
        adj.push_back({});
        base_indegree.push_back(0);
        return id;
    }

    /**
     * @brief Create a directed edge between two nodes (dependency).
     * @param from The prerequisite node.
     * @param to The dependent node (will wait for 'from').
     */
    void add_edge(NodeID from, NodeID to) {
        if (from >= nodes.size() || to >= nodes.size()) return;
        adj[from].push_back(to);
        base_indegree[to]++;
    }

    /**
     * @brief Prepare the graph for execution by resetting atomic counters.
     * Automatically called by execute_parallel.
     */
    void reset() {
        if (current_indegree.size() != nodes.size()) {
            current_indegree = std::vector<std::atomic<int>>(nodes.size());
        }
        for (size_t i = 0; i < nodes.size(); ++i) {
            current_indegree[i].store(base_indegree[i]);
        }
        remaining.store(nodes.size());
    }

    /**
     * @brief Set a value to the node's output port.
     * @param id The ID of the node producing the data.
     * @param value Any value to be shared with successors.
     */
    void set_port_value(NodeID id, std::any value) {
        nodes[id].output = std::move(value);
    }

    /**
     * @brief Retrieve a value from a node's output port.
     * @tparam ValType The expected type of the stored data.
     * @param id The ID of the node that produced the data.
     * @return ValType The casted value.
     */
    template <typename ValType>
    ValType get_port_value(NodeID id) {
        return std::any_cast<ValType>(nodes[id].output);
    }

    /**
     * @brief Execute the graph in parallel using a thread pool.
     * @tparam Pool The type of thread pool (must implement enqueue).
     * @tparam Func The executor lambda type: void(NodeID, T&).
     * @param pool Reference to the thread pool.
     * @param executor Lambda defining the work to be done for each node.
     */
    template <typename Pool, typename Func>
    void execute_parallel(Pool& pool, Func executor) {
        if (nodes.empty()) return;

        reset();

        std::mutex wait_mtx;
        std::condition_variable wait_cv;

        // Task scheduling logic
        std::function<void(NodeID)> dispatch = [&](NodeID u) {
            // Execute the user-defined work
            executor(u, nodes[u].data);

            // Resolve dependencies for successors
            for (NodeID v : adj[u]) {
                // If this was the last dependency, enqueue successor
                if (current_indegree[v].fetch_sub(1) == 1) {
                    pool.enqueue([&dispatch, v] { dispatch(v); });
                }
            }

            // Decrement global counter and notify if finished
            if (remaining.fetch_sub(1) == 1) {
                std::lock_guard<std::mutex> lk(wait_mtx);
                wait_cv.notify_all();
            }
            };

        // Start initial nodes (those with 0 incoming dependencies)
        for (NodeID i = 0; i < nodes.size(); ++i) {
            if (base_indegree[i] == 0) {
                pool.enqueue([&dispatch, i] { dispatch(i); });
            }
        }

        // Synchronize and wait for the entire graph to finish
        std::unique_lock<std::mutex> lk(wait_mtx);
        wait_cv.wait(lk, [this] { return remaining.load() == 0; });
    }

    /**
     * @brief Access node data by ID.
     */
    T& operator[](NodeID id) { return nodes[id]; }

    /**
     * @brief Get the total number of nodes in the graph.
     */
    size_t size() const { return nodes.size(); }
};