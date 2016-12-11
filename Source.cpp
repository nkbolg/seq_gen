// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <map>
#include <string>
#include <memory>
#include <numeric>
#include <array>
#include <cmath>
#include <functional>

#include "short_alloc.h"

using namespace std;

template <int MIN, int MAX>
int getRand()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(MIN, MAX);
    return dis(gen);
}

int getRand(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

bool flip(double p = 0.5)
{
    int pp = int(p * 100);
    return getRand<0,100-1>() < pp;
}

template <size_t N>
int randFrom( const array<int, N> &values, array<int, N> probs)
{
    int sum = 0;
    for_each(probs.begin(), probs.end(), [&sum](int &val) {
        val += sum;
        sum = val;
    });
    const int rand = getRand(0, sum-1);
    size_t targetPos = distance(probs.begin(), 
        find_if(probs.begin(), probs.end(), 
            [rand](int element) { return element > rand; }));
    return values[targetPos];
}

template <class T, size_t N = 80>
auto& arenaFor()
{
    static arena<sizeof(T) * N, alignof(T)> a{};
    return a;
}

class Node;
using NodePtr = Node*;
NodePtr generate_operations();
using GeneratorFn = std::function<NodePtr()>;

class Node
{
public:
    virtual ~Node() = default;
    virtual int eval(size_t n, int xp, int xpp) const = 0;
    virtual int size() const = 0;
    virtual void print(ostream &strm) const = 0;
    virtual NodePtr getCopy() const = 0;
    virtual void mutate(int &mut_ind, GeneratorFn gen_fn) = 0;
    virtual NodePtr getByIndex(int &index) = 0;
};


// 0-9
class Value : public Node
{
public:
    Value(int _data) : data(_data) {}
    ~Value() = default;
    Value(const Value &val) = default;
    int eval(size_t n, int xp, int xpp) const override { return data; }
    int size() const override { return 1; }
    void print(ostream &strm) const override { strm << data << " "; }

    Node* getCopy() const override
    {
        return static_cast<Node*>(new Value(*this));
    }

    virtual void mutate(int &mut_ind, GeneratorFn gen_fn) {
        mut_ind--;
    }

    virtual NodePtr getByIndex(int &index)
    {
        if (index == 0)
        {
            return this;
        }
        else
        {
            index--;
            return nullptr;
        }
    }

    void* Value::operator new (size_t count)
    {
        return arenaFor<Value>().allocate<alignof(Value)>(count);
    }
    void Value::operator delete(void *ptr)
    {
        arenaFor<Value>().deallocate((char*)ptr, sizeof(Value));
    }
private:
    int data;
};

enum class VariableType : size_t
{
    N,
    XP,
    XPP
};

map<VariableType, string> VarTypeToStr = { {VariableType::N, "N"}, {VariableType::XP, "XP"}, {VariableType::XPP, "XPP"} };

//n, [n - 1], [n - 2]
class Variable : public Node
{
public:
    Variable(VariableType _type) : type(_type) {}
    ~Variable() = default;
    Variable(const Variable &val) = default;
    virtual int eval(size_t n, int xp, int xpp) const override {
        switch (type)
        {
        case VariableType::N:
            return (int)n;
            break;
        case VariableType::XP:
            return xp;
            break;
        case VariableType::XPP:
            return xpp;
            break;
        default:
            throw exception("Shit happens 0");
            break;
        }
        return 0;
    }
    virtual int size() const override { return 1; }
    virtual void print(ostream &strm) const override { strm << VarTypeToStr[type] << " "; }

    Node* getCopy() const override
    {
        return static_cast<Node*>(new Variable(*this));
    }

    virtual void mutate(int &mut_ind, GeneratorFn gen_fn) {
        mut_ind--;
    }

    virtual NodePtr getByIndex(int &index)
    {
        if (index == 0)
        {
            return this;
        }
        else
        {
            index--;
            return nullptr;
        }
    }

    void* Variable::operator new (size_t count)
    {
        return arenaFor<Variable>().allocate<alignof(Variable)>(count);
    }
    void Variable::operator delete(void *ptr)
    {
        arenaFor<Variable>().deallocate((char*)ptr, sizeof(Variable));
    }

private:
    VariableType type;
};

enum class OperationType : size_t
{
    plus,
    minus,
    mul
};

map<OperationType, string> OpTypeToStr = { {OperationType::plus, "+"}, {OperationType::minus, "-"}, {OperationType::mul, "*"} };

class Operation : public Node
{
public:
    Operation(OperationType _operation, const std::pair<NodePtr, NodePtr> &_nodeStg) : operation(_operation), nodeStg(_nodeStg) {}
    ~Operation() 
    { 
        delete nodeStg.first;
        delete nodeStg.second;
    }
    Operation(const Operation &val) : operation(val.operation), nodeStg(make_pair(val.nodeStg.first->getCopy(), val.nodeStg.second->getCopy()))
    {
    }
    virtual int eval(size_t n, int xp, int xpp) const override {
        switch (operation)
        {
        case OperationType::plus:
            return nodeStg.first->eval( n, xp, xpp) + nodeStg.second->eval( n, xp, xpp);
            break;
        case OperationType::minus:
            return nodeStg.first->eval( n, xp, xpp) - nodeStg.second->eval( n, xp, xpp);
            break;
        case OperationType::mul:
            return nodeStg.first->eval( n, xp, xpp) * nodeStg.second->eval( n, xp, xpp);
            break;
        default:
            throw exception("Shit happens 1");
            break;
        }
        return 0;
    }
    virtual int size() const override { return 1 + nodeStg.first->size() + nodeStg.second->size(); }
    virtual void print(ostream &strm) const override {
        strm << "( " << OpTypeToStr[operation] << " ";
        nodeStg.first->print(strm);
        nodeStg.second->print(strm);
        strm << ") ";
    }

    Node* getCopy() const override
    {
        return static_cast<Operation*>(new Operation(*this));
    }

    virtual void mutate(int &mut_ind, GeneratorFn gen_fn)
    {
        mut_ind--;
        if (mut_ind == 0)
        {
            delete nodeStg.first;
            nodeStg.first = gen_fn();
            return;
        }

        int sz_left = nodeStg.first->size();
        if (sz_left > mut_ind)
        {
            nodeStg.first->mutate(mut_ind, gen_fn);
        }
        else
        {
            mut_ind -= sz_left;
            if (mut_ind == 0)
            {
                delete nodeStg.second;
                nodeStg.second = gen_fn();
                return;
            }
            nodeStg.second->mutate(mut_ind, gen_fn);
        }
    }

    virtual NodePtr getByIndex(int &index)
    {
        if (index == 0)
        {
            return this;
        }
        else
        {
            index--;
            NodePtr left =  nodeStg.first->getByIndex(index);
            return left != nullptr ? left : nodeStg.second->getByIndex(index);
        }
    }

    void* Operation::operator new (size_t count)
    {
        return arenaFor<Operation>().allocate<alignof(Operation)>(count);
    }
    void Operation::operator delete(void *ptr)
    {
        arenaFor<Operation>().deallocate((char*)ptr, sizeof(Operation));
    }
private:
    OperationType operation;
    std::pair<NodePtr, NodePtr> nodeStg;
};


NodePtr generate_operations()
{
    NodePtr root = nullptr;
    if (flip(0.6))
    {//добавить новое непосредственное значение
        switch (getRand<0, 1>())
        {
        case 0:
            root = new Value(getRand< 0, 9>());
            break;
        case 1:
            root = new Variable((VariableType)(randFrom<3>({ 0,1,2 }, {50, 1, 1})));
            break;
        }
    }
    else
    {//добавить новый узел вычислений
        const auto pair = make_pair(generate_operations(), generate_operations());
        root = new Operation((OperationType)getRand<0, 2>(), pair);
    }
    return root;
}

template <size_t N>
array<int, N> calculate(const NodePtr operation_tree, int xpp, int xp)
{
    constexpr size_t count = N;
    array<int, N> res_seq;
    res_seq[0] = xpp;
    res_seq[1] = xp;
    for (size_t i = 2; i < count; i++)
    {
        res_seq[i] = operation_tree->eval(i+1, res_seq[i-1], res_seq[i-2]);
    }
    return res_seq;
}

template <class T, size_t N>
double distance(const array<T, N> &lhs, const array<T, N> &rhs)
{
    double dist = 0.;
    for (size_t i = 0; i < N; i++)
    {
        dist += pow(lhs[i] - rhs[i], 2);
    }
    dist = sqrt(dist);
    return dist;
}

template <size_t N>
unique_ptr<Node> dumb_random_search(const array <int, N> &target)
{
    array<int, N> result{};
    unique_ptr<Node> root;
    while (true)
    {
        root.reset(generate_operations());
        result = calculate<N>(root.get(), target[0], target[1]);
        if (target == result)
        {
            break;
        }
    }
    return root;
}

void mutate(unique_ptr<Node> &root)
{
    int sz = root->size();
    int mut_ind = getRand(0, sz-1);
    if (mut_ind == 0)
    {
        root.reset(generate_operations());
    }
    else
    {
        //TODO: change to taking NodePtr
        root->mutate(mut_ind, generate_operations);
    }
}

NodePtr hybridise(NodePtr p0, NodePtr p1)
{
    int sz0 = p0->size();
    int sz1 = p1->size();

    int get_node = getRand(0, sz0-1);
    int set_node = getRand(0, sz1-1);

    NodePtr node = nullptr;
    if (get_node == 0)
    {
        node = p0->getCopy();
    }
    else
    {
        node = p0->getByIndex(get_node)->getCopy();
    }

    if (set_node == 0)
    {
        return node;
    }
    else
    {
        NodePtr result_node = p1->getCopy();
        result_node->mutate(set_node, [node] {return node; });
        return result_node;
    }

}

template <size_t N>
unique_ptr<Node> mutating_search(const array <int, N> &target)
{
    unique_ptr<Node> winner = nullptr;
    array<int, N> result {};
    constexpr size_t gens_number = 256;
    array<unique_ptr<Node>, gens_number> gens_b0, gens_b1, *gens, *new_gens;
    array<pair<double,size_t>, gens_number> distances;

    constexpr size_t elite = gens_number / 4; //-V112
    constexpr size_t parents = gens_number / 8;
    constexpr size_t children = gens_number / 4; //-V112
    constexpr size_t new_ones = gens_number - elite - children;

    constexpr int max_nodes_number = 30;

    gens = &gens_b0;
    new_gens = &gens_b1;

    for_each(begin(*gens), end(*gens), [](auto &genPtr) { genPtr.reset(generate_operations()); });

    while (true)
    {
        for_each(begin(*gens), end(*gens), [max_nodes_number](auto &genPtr) {
            if ( genPtr->size() > max_nodes_number )
            {
                genPtr.reset(generate_operations());
            }
        });

        for (size_t i = 0; i < gens->size(); ++i)
        {
            auto &gen = (*gens)[i];
            result = calculate<N>(gen.get(), target[0], target[1]);
            double m_distance = distance(result, target);
            distances[i] = make_pair(m_distance, i);
            if (m_distance <= 2.)
            {
                winner = move(gen);
                return winner;
            }
        }

        std::sort(begin(distances), end(distances), [](const auto &l, const auto &r) {
            return l.first < r.first;
        });
        size_t i = 0;
        for (; i < children; i++)
        {
            size_t parent0_index = (size_t)getRand<0, parents-1>();
            size_t parent1_index = (size_t)getRand<0, parents-1>();
            auto newGen = unique_ptr<Node>(hybridise((*gens)[distances[parent0_index].second].get(), (*gens)[distances[parent1_index].second].get()));
            mutate(newGen);
            (*new_gens)[i] = move(newGen);
        }
        for (size_t j = 0; i < elite+children; i++, j++)
        {
            (*new_gens)[i] = move((*gens)[distances[j].second]);
        }
        for (; i < new_ones+elite+children; i++)
        {
            (*new_gens)[i] = unique_ptr<Node>(generate_operations());
        }
        swap(gens, new_gens);
    }
}

template <size_t N>
void logOperations(ostream &strm, size_t millisecs, const unique_ptr<Node> &root, const array<int, N> &target)
{
    strm << "Function: " << endl;
    root->print(strm);
    strm << endl;

    array<int, N> result{};
    result = calculate<N>(root.get(), target[0], target[1]);
    
    strm << "Target: ";
    for (const auto& elem : target)
    {
        strm << elem << " ";
    }
    strm << endl;

    strm << "Result: ";
    for (const auto& elem : result)
    {
        strm << elem << " ";
    }
    strm << endl;

    strm << "Time: " << millisecs << " milliseconds" << endl << endl; //-V128
}

void measure(int n = 100)
{
    ofstream logfile("measure.txt", ios_base::app);

    logfile << "Count: " << n << endl << endl;

    logfile << "Mutating search" << endl << endl;

    size_t total = 0;

    constexpr array<int, 8> target{ 0, 4, 30, 120, 340, 780, 1554, 2800 };

    for (int i = 0; i < n; i++)
    {
        auto t_start = chrono::high_resolution_clock::now();
        auto root = mutating_search(target);
        auto t_end = chrono::high_resolution_clock::now();
        auto millisecs = chrono::duration_cast<chrono::milliseconds>(t_end - t_start);
        total += millisecs.count();
        logOperations(cout, (size_t)millisecs.count(), root, target);
        logOperations(logfile, (size_t)millisecs.count(), root, target);
    }

    logfile << endl << "Total: " << total << endl; //-V128


    total = 0;

    for (int i = 0; i < n; i++)
    {
        auto t_start = chrono::high_resolution_clock::now();
        auto root = dumb_random_search(target);
        auto t_end = chrono::high_resolution_clock::now();
        auto millisecs = chrono::duration_cast<chrono::milliseconds>(t_end - t_start);
        total += millisecs.count();
        logOperations(cout, (size_t)millisecs.count(), root, target);
        logOperations(logfile, (size_t)millisecs.count(), root, target);
    }

    logfile << endl << "Total: " << total << endl; //-V128
}

int main()
{
    /*ofstream logfile("out.txt", ios_base::app);
    auto t_start = chrono::high_resolution_clock::now();

    //map<int, int> fre;
    //for (int i = 0; i < 1'000'000; i++)
    //{
    //    //int rn = randFrom<3>({ 1,2,3 }, { 50,1,1 });
    //    int rn = getRand(0, 0);
    //    fre[rn]++;
    //}

    //4
    constexpr array<int, 8> target{ 0, 4, 30, 120, 340, 780, 1554, 2800 };

    //3
    //constexpr array<int, 6> target{ 0, 3, 14, 39, 84, 155 };

    //constexpr array<int, 6> target{ 0, 4, 1, 2, 3, 4 };

    //constexpr array<int, 4> target{ 0, 0, 500, 25 };

    //auto root = dumb_random_search(target);
    auto root = mutating_search(target);


    auto t_end = chrono::high_resolution_clock::now();

    auto millisecs = chrono::duration_cast<chrono::milliseconds>(t_end - t_start);
    logOperations(cout, (size_t)millisecs.count(), root, target);
    logOperations(logfile, (size_t)millisecs.count(), root, target);

    */
    measure();
    return 0;
}
