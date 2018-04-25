//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H


#include <memory>
#include <cmath>
#include <queue>
#include "SimpleGraph.h"
#include "RPQTree.h"
#include "Evaluator.h"
#include "Graph.h"
#include "SimpleEstimator.h"

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

    // Caches
    std::map<std::string, std::shared_ptr<SimpleGraph>> projectCache;
    std::map<std::string, std::shared_ptr<SimpleGraph>> joinCache;
    std::map<std::string, cardStat> resultCache;

    // Queues for caches
    std::deque<std::string> projectQueue;
    std::deque<std::string> joinQueue;
    std::deque<std::string> resultQueue;

    const static int PROJECT_CACHE_SIZE = 20;
    const static int JOIN_CACHE_SIZE = 20;
    const static int RESULT_CACHE_SIZE = 50;
    const int NUM_QUERIES_PREPARE = 10;

public:
    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEvaluator() = default;

    void prepare() override ;
    cardStat evaluate(RPQTree *query) override ;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<SimpleGraph> evaluate_aux(RPQTree *q);
    static std::shared_ptr<SimpleGraph> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g);
    static std::shared_ptr<SimpleGraph> join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right);

    static std::string findQueryString(RPQTree *q);

    template <typename Value>
    bool checkUpdateCache(std::string &query, std::map<std::string, Value> &cache, std::deque<std::string> &queue, const int cache_size);
    template <typename Value>
    void insertIntoCache(std::string &query, std::map<std::string, Value> &cache, std::deque<std::string> &queue, Value &result, const int cache_size);

    static cardStat computeStats(std::shared_ptr<SimpleGraph> &g);

    std::vector<std::string> getPrioritizedAST_aux(std::vector<std::string> queryParts);

    RPQTree *getPrioritizedAST(RPQTree *query);
};


#endif //QS_SIMPLEEVALUATOR_H
