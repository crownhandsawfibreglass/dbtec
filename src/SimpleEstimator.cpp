//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
}

// Return the query as a vector of labels, ignoring parentheses
std::vector<std::string> SimpleEstimator::labelDirectionsFromQuery(RPQTree *t) {
    std::vector<std::string> edges;

    if (t->isLeaf()) {
        edges.emplace_back(t->data);
    } else {
        auto left_edges = labelDirectionsFromQuery(t->left);
        auto right_edges = labelDirectionsFromQuery(t->right);
        edges.insert(edges.end(), left_edges.begin(), left_edges.end());
        if (t->data != "/") {
            edges.emplace_back(t->data);
        }
        edges.insert(edges.end(), right_edges.begin(), right_edges.end());
    }
    return edges;
}

void SimpleEstimator::prepare() {

    // Add initial entries to possibilities
    for (int vertex = 0; vertex < graph->getNoVertices(); vertex++) {
        for (int label = 0; label < graph->getNoLabels(); label++) {
            possibilities[vertex][label]["+"] = 0;
            possibilities[vertex][label]["-"] = 0;
        }
    }

    // Calculate values for possibilities
    for (int vertex = 0; vertex < graph->getNoVertices(); vertex++) {
        for (auto const& labelAndDestination : graph->adj[vertex]) {
            possibilities[vertex][labelAndDestination.first]["+"] += 1;
            possibilities[labelAndDestination.second][labelAndDestination.first]["-"] += 1;
        }
    }

    // Add initial entries to probabilities
    for (int label1 = 0; label1 < graph->getNoLabels(); label1++) {
        for (int label2 = 0; label2 < graph->getNoLabels(); label2++) {
            probabilities[label1]["+"][label2]["+"] = 0;
            probabilities[label1]["+"][label2]["-"] = 0;
            probabilities[label1]["-"][label2]["+"] = 0;
            probabilities[label1]["-"][label2]["-"] = 0;
        }
    }

    // Calculate values for probabilities, using possibilities
    for (int vertex1 = 0; vertex1 < graph->getNoVertices(); vertex1++) {
        for (auto const& labelAAndVertex2 : graph->adj[vertex1]) {
            auto labelA = labelAAndVertex2.first;
            auto vertex2 = labelAAndVertex2.second;

            // With labelA, we can go from vertex1 to vertex2 in the positive direction. This means:

            // probabilities[labelA]["+"][labelB]["+"] should be increased with possibilities[vertex2][labelB]["+"] for
            // all possible labelB
            // probabilities[labelA]["+"][labelB]["-"] should be increased with possibilities[vertex2][labelB]["-"] for
            // all possible labelB
            // probabilities[labelA]["-"][labelB]["+"] should be increased with possibilities[vertex1][labelB]["+"] for
            // all possible labelB
            // probabilities[labelA]["-"][labelB]["-"] should be increased with possibilities[vertex1][labelB]["-"] for
            // all possible labelB

            for (int labelB = 0; labelB < graph->getNoLabels(); labelB++) {
                probabilities[labelA]["+"][labelB]["+"] += possibilities[vertex2][labelB]["+"];
                probabilities[labelA]["+"][labelB]["-"] += possibilities[vertex2][labelB]["-"];
                probabilities[labelA]["-"][labelB]["+"] += possibilities[vertex1][labelB]["+"];
                probabilities[labelA]["-"][labelB]["-"] += possibilities[vertex1][labelB]["-"];
            }
        }
    }

    // Now we have to divide all final entries in probabilities with the total number of edges we have with the same
    // label
    for (int label = 0; label < graph->getNoLabels(); label++) {
        labelCounts[label] = 0;
    }
    for (int vertex1 = 0; vertex1 < graph->getNoVertices(); vertex1++) {
        for (auto const& labelAndVertex2 : graph->adj[vertex1]) {
            labelCounts[labelAndVertex2.first] += 1;
        }
    }

    // Do the division
    for (int label1 = 0; label1 < graph->getNoLabels(); label1++) {
        for (int label2 = 0; label2 < graph->getNoLabels(); label2++) {
            probabilities[label1]["+"][label2]["+"] /= labelCounts[label1];
            probabilities[label1]["+"][label2]["-"] /= labelCounts[label1];
            probabilities[label1]["-"][label2]["+"] /= labelCounts[label1];
            probabilities[label1]["-"][label2]["-"] /= labelCounts[label1];
        }
    }

}

cardStat SimpleEstimator::estimate(RPQTree *q) {


    std::vector<std::string> labelDirections = labelDirectionsFromQuery(q);
    std::string firstLabelDirection = labelDirections.front();
    auto firstLabel = std::stoi(firstLabelDirection.substr(0, firstLabelDirection.size() - 1));

    auto noPaths = (float) labelCounts[firstLabel];

    for (int i = 1; i < labelDirections.size(); i++) {
        auto prevLabel = std::stoi(labelDirections[i-1].substr(0, labelDirections[i-1].size() - 1));
        std::string prevDirection = std::string(1, labelDirections[i-1].back());
        auto currentLabel = std::stoi(labelDirections[i].substr(0, labelDirections[i].size() - 1));
        std::string currentDirection = std::string(1, labelDirections[i].back());

        noPaths *= probabilities[prevLabel][prevDirection][currentLabel][currentDirection];
    }

    uint32_t noOut = 1000;
    uint32_t noIn = 1000;

    return cardStat {noOut, (uint32_t) noPaths, noIn};
}