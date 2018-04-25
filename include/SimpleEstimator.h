//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"
#include <iostream>
#include <cstring>

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;

    std::vector<std::string> labelDirectionsFromQuery(RPQTree *t);

    // possibilities[vertex][label][direction] gives us the possibilities map for that node tells us how many
    // edges with the given label are outgoing from the given vertex with in the given direction
    std::map<int, std::map<int, std::map<std::string, int>>> possibilities;

    // probabilities[label1][direction1][label2][direction2] gives us the number of label2 in direction2 that can
    // come after label1 in direction1, per total number of label1 in the graph
    std::map<int, std::map<std::string, std::map<int, std::map<std::string, float>>>> probabilities;

    // labelCount[label] should give the total number of edges with the given label
    std::map<int, int> labelCounts;
};


#endif //QS_SIMPLEESTIMATOR_H
