#ifndef TREE_HPP
#define TREE_HPP
#include <vector>
#include <mutex>
#include "stellarObjects/stellarObject.hpp"
#include "helpers/treeCodeNode.hpp"
#include "graphicInterface/renderer.hpp"

#define DOUBLE_MAX_VALUE 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0
#define DOUBLE_MIN_VALUE -179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0
#define LONG_DOUBLE_MAX_VALUE 1.18973e+4932L
#define LONG_DOUBLE_MIN_VALUE -1.18973e+4932L

#define TREE_APPROACH true

#define STELLAR_UPDATE_THREAD_COUNT 1
//#define LOCAL_UPDATE_THREAD_COUNT 12
#define LOCAL_UPDATE_THREAD_COUNT 1


class Tree{
private:
    TreeCodeNode *root;

    bool treeType;

    // ------------------------------------------------------- TODO -------------------------------------------------------
    // Create treeCodeNode pool
    // Here, treeCodeNodes get create on the fly, not taken from a pool
    // std::vector<TreeCodeNode*> *allTreeCodeNodes;
    // std::vector<int> *freeTreeCodeNodes;
    std::vector<StellarObject*> *objectsInTree;

    std::mutex *currentlyUpdatingOrDrawingLock;

public:
    Tree(std::vector<StellarObject*> *objectsInTree, std::mutex *currentlyUpdatingOrDrawingLock);

    void buildTree(long double timestep = 0);
    void destroyTree();
    void update(long double timestep, Renderer *renderer);
    std::vector<StellarObject*> *getObjectsInTree();

    TreeCodeNode *getRoot();

    // Idea behind force calculation: go down to node with less than 20 (to be adjusted) objects 
    // and use its center of mass as origin for force calculation, such that we can reuse the quadrupole
};

void updateStellarAccelerationTreeMultiThread(Tree *tree, int begin, int end);
void updateLocalAccelerationTreeMultiThread(Tree *tree, int begin, int end);

#endif
