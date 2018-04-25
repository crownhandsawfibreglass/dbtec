//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include <set>
#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

bool debug = false;
SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary

    std::map<std::string, int> counts;
    // Calculate values for possibilities
    for (int vertex = 0; vertex < graph->getNoVertices(); vertex++) {
        for (auto const& labelAndDestination : graph->adj[vertex]) {
            counts[std::to_string(labelAndDestination.first) + "+"] += 1;
            counts[std::to_string(labelAndDestination.first) + "-"] += 1;
        }
    }

    /*
    std::map<std::string, int> edges;
    // Calculate values for possibilities
    for (int vertex = 0; vertex < graph->getNoVertices(); vertex++) {
        for (auto const& labelAndDestination : graph->adj[vertex]) {
            edges[std::to_string(vertex) + std::to_string(labelAndDestination.first) + "+"] += 1;
            edges[std::to_string(labelAndDestination.second) + std::to_string(labelAndDestination.first) + "-"] += 1;
        }
    }

    std::map<std::string, int> counts;
    // Calculate values for probabilities, using possibilities
    for (int vertex1 = 0; vertex1 < graph->getNoVertices(); vertex1++) {
        for (auto const& labelAAndVertex2 : graph->adj[vertex1]) {
            auto labelA = labelAAndVertex2.first;
            auto vertex2 = labelAAndVertex2.second;

            for (int labelB = 0; labelB < graph->getNoLabels(); labelB++) {
                counts[std::to_string(labelA) + "+/" + std::to_string(labelB) + "+"] += edges[std::to_string(vertex2) + std::to_string(labelB) + "+"];
                counts[std::to_string(labelA) + "+/" + std::to_string(labelB) + "-"] += edges[std::to_string(vertex2) + std::to_string(labelB) + "-"];
                counts[std::to_string(labelA) + "-/" + std::to_string(labelB) + "+"] += edges[std::to_string(vertex1) + std::to_string(labelB) + "+"];
                counts[std::to_string(labelA) + "-/" + std::to_string(labelB) + "-"] += edges[std::to_string(vertex1) + std::to_string(labelB) + "-"];
            }
        }
    }*/

    typedef std::function<bool(std::pair<std::string, int>, std::pair<std::string, int>)> Comparator;
    // Defining a lambda function to compare two pairs. It will compare two pairs using second field
    Comparator compFunctor =
            [](std::pair<std::string, int> elem1 ,std::pair<std::string, int> elem2)
            {
                return elem1.second > elem2.second;
            };

    // Declaring a set that will store the pairs using above comparision logic
    std::set<std::pair<std::string, int>, Comparator> setOfWords(counts.begin(), counts.end(), compFunctor);

    // Iterate over a set using range base for loop
    // It will display the items in sorted order of values
    int cnt = 0;
    for (std::pair<std::string, int> element : setOfWords) {
        auto q = RPQTree::strToTree(element.first);
        evaluate(q);
        cnt++;
        if (cnt >= NUM_QUERIES_PREPARE) {
            break;
        }
    }
}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats {};

    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }

    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    } else {
        // going backward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->reverse_adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto labelTarget : left->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }

    return out;
}

std::string SimpleEvaluator::findQueryString(RPQTree *q) {
    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        return q->data;
    }

    if(q->isConcat()) {
        auto l = findQueryString(q->left);
        auto r = findQueryString(q->right);
        return l + r;
    }
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        bool inCache = SimpleEvaluator::checkUpdateCache(q->data, projectCache, projectQueue, PROJECT_CACHE_SIZE);
        if (inCache) {
            return projectCache[q->data];
        }

        // Query not found in the cache, evaluate query and insert it into the cache
        auto p = SimpleEvaluator::project(label, inverse, graph);
        SimpleEvaluator::insertIntoCache(q->data, projectCache, projectQueue, p, PROJECT_CACHE_SIZE);
        return p;
    }

    if(q->isConcat()) {
        auto qString = SimpleEvaluator::findQueryString(q);

        bool inCache = SimpleEvaluator::checkUpdateCache(q->data, joinCache, joinQueue, JOIN_CACHE_SIZE);
        if (inCache) {
            return joinCache[qString];
        }

        // Query not found in the cache, evaluate query and insert it into the cache
        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);
        // join left with right

        auto j = SimpleEvaluator::join(leftGraph, rightGraph);
        SimpleEvaluator::insertIntoCache(qString, joinCache, joinQueue, j, JOIN_CACHE_SIZE);
        return j;
    }

    return nullptr;
}

template<typename Value>
bool SimpleEvaluator::checkUpdateCache(std::string &query, std::map<std::string, Value> &cache, std::deque<std::string> &queue, const int cache_size) {
    // Try to find query in cache
    auto findQ = std::find(queue.begin(), queue.end(), query);

    // If query is found in cache, move it to the back of the queue and return the value
    if (findQ != queue.end()) {
        if (debug) {
            std::cout << "Cache hit: " + query + ", moving it to the back of the queue" << "\n";
        }
        queue.erase(findQ);
        queue.push_back(query);
        return true;
    }

    return false;
}

template <typename Value>
void SimpleEvaluator::insertIntoCache(std::string &query, std::map<std::string, Value> &cache, std::deque<std::string> &queue, Value &result, const int cache_size) {
    if (debug) {
        std::cout << "Inserting new value " + query + " into cache" << "\n";
    }
    queue.push_back(query);
    cache[query] = result;

    // If cache size exceeds MAX_CACHE_SIZE remove LRU entry
    if (cache.size() > cache_size) {
        // Get value of first entry in the queue, should be lru
        std::string lru = queue.front();
        // Remove the first entry from the queue
        queue.pop_front();
        // Remove the entry from the cache
        cache.erase(lru);

        if (debug) {
            std::cout << "Max cache size exceeded, removed " + lru + " from cache" << "\n";
        }
    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    std::string qString = SimpleEvaluator::findQueryString(query);

    if (debug) {
        std::cout << "\n\n" << "#### DEBUG BLOCK ####" << "\n";
        std::cout << "Evaluating query: " + qString << "\n";
    }

    bool inCache = SimpleEvaluator::checkUpdateCache(qString, resultCache, resultQueue, RESULT_CACHE_SIZE);
    if (inCache) {
        return resultCache[qString];
    }

    auto pq = getPrioritizedAST(query);

    // Query not found in the cache, evaluate query and insert it into the cache
    auto res = evaluate_aux(pq);
    delete pq;

    cardStat resStats = SimpleEvaluator::computeStats(res);

    SimpleEvaluator::insertIntoCache(qString, resultCache, resultQueue, resStats, RESULT_CACHE_SIZE);
    return resStats;
}

RPQTree* SimpleEvaluator::getPrioritizedAST(RPQTree *query) {
    auto queryParts = SimpleGraph::inOrderNodesClean(query);
    auto queryString = getPrioritizedAST_aux(queryParts)[0];
    return RPQTree::strToTree(queryString);
}

std::vector<std::string> SimpleEvaluator::getPrioritizedAST_aux(std::vector<std::string> queryParts) {
    if (queryParts.size() == 1) {
        return queryParts;
    }

    std::vector<int> cardinalities;
    cardinalities.resize(queryParts.size()-1);

    for (auto i = 1; i < queryParts.size(); i++) {
        std::string query = queryParts[i-1] + "/" + queryParts[i];
        auto queryTree = RPQTree::strToTree(query);
        cardinalities[i-1] = est->estimate(queryTree).noPaths;
        delete queryTree;
    }

    int min_cardinality = 9999999;
    int min_cardinality_i = 0;

    for (auto i = 0; i < queryParts.size()-1; i++) {
        auto cardinality = cardinalities[i];
        if (cardinality < min_cardinality) {
            min_cardinality_i = i;
            min_cardinality = cardinality;
        }
    }

    std::vector<std::string> newQueryParts;
    newQueryParts.resize(queryParts.size() - 1);

    for (auto i = 0; i < newQueryParts.size(); i++) {
        if (i < min_cardinality_i) {
            newQueryParts[i] = queryParts[i];
        } else if (i == min_cardinality_i) {
            newQueryParts[i] = "(" + queryParts[i] + "/" + queryParts[i+1] + ")";
        } else if (i >= min_cardinality_i + 1) {
            newQueryParts[i] = queryParts[i+1];
        }
    }
    return getPrioritizedAST_aux(newQueryParts);
}

