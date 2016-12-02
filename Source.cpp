// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <vector>
#include <random>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <numeric>
#include <array>
#include <cmath>

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

bool flip(double p = 0.5)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis;
    return dis(gen) < p;
}

template <size_t N>
int randFrom( const array<int, N> &values, array<double, N> probs)
{
    double sum = 0.;
    for_each(probs.begin(), probs.end(), [&sum](double &val) {
        val += sum;
        sum = val;
    });
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, sum);
    const double rand = dis(gen);
    size_t targetPos = distance(probs.begin(), 
        find_if(probs.begin(), probs.end(), 
            [rand](double element) { return element > rand; }));
    return values[targetPos];
}

template <class T, size_t N = 80>
auto& arenaFor()
{
    static arena<sizeof(T) * N, alignof(T)> a{};
    return a;
}

class Node
{
public:
    virtual ~Node() = default;
    virtual int eval(size_t n, int xp, int xpp) const = 0;
    virtual int size() const = 0;
    virtual void print() const = 0;
};

using NodePtr = Node*;

// 0-9
class Value : public Node
{
public:
    Value(int _data) : data(_data) {}
    ~Value() = default;
    virtual int eval(size_t n, int xp, int xpp) const override { return data; }
    virtual int size() const override { return 1; }
    virtual void print() const override { cout << data << " "; }

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
    virtual void print() const override { cout << VarTypeToStr[type] << " "; }

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
    virtual void print() const override {
        cout << "( " << OpTypeToStr[operation] << " ";
        nodeStg.first->print();
        nodeStg.second->print();
        cout << ") ";
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
    if (flip(0.7))
    {//добавить новое непосредственное значение
        switch (getRand<0, 1>())
        {
        case 0:
            root = new Value(getRand< 0, 9>());
            break;
        case 1:
            root = new Variable((VariableType)(randFrom<3>({ 0,1,2 }, {50, 25, 25})));
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

NodePtr mutate(NodePtr p0, NodePtr p1)
{
    int sz0 = p0->size();
    int sz1 = p1->size();
    //TODO: do something
    return{};
}

template <size_t N>
unique_ptr<Node> mutating_search(const array <int, N> &target)
{
    unique_ptr<Node> winner = nullptr;
    array<int, N> result {};
    constexpr size_t gens_number = 4;
    array<unique_ptr<Node>, gens_number> gens;
    array<pair<double,size_t>, gens.size()> distances;
    size_t mutants = 2;

    for_each(begin(gens), end(gens), [](auto &genPtr) { genPtr.reset(generate_operations()); });

    while (true)
    {
        for (size_t i = 0; i < gens.size(); ++i)
        {
            auto &gen = gens[i];
            result = calculate<N>(gen.get(), target[0], target[1]);
            double m_distance = distance(result, target);
            distances[i] = make_pair(m_distance, i);
            if (m_distance <= 1.)
            {
                winner = move(gen);
                return winner;
            }
        }

        std::sort(begin(distances), end(distances), [](const auto &l, const auto &r) {
            return l.first < r.first;
        });

        /*unique_ptr<Node> parent0 = move(gens[distances[0].second]);
        unique_ptr<Node> parent1 = move(gens[distances[1].second]);

        for_each(begin(gens), begin(gens) + mutants, [&parent0, &parent1](auto &gen) {
            gen.reset(mutate(parent0.get(), parent1.get()));
        });*/
        for_each(begin(gens) + mutants, end(gens), [](auto &gen) {
            gen.reset(generate_operations());
        });


    }
}

int main()
{
    //map<int, int> fre;
    //for (int i = 0; i < 1000000; i++)
    //{
    //    int rn = randFrom<3>({ 1,2,3 }, { 25,1,1 });
    //    fre[rn]++;
    //}
    
    constexpr array<int, 5> target{ 1, 1, 2, 3, 5};

    auto root = dumb_random_search(target);
    //auto root = mutating_search(target);

    array<int, target.size()> result{};
    result = calculate<target.size()>(root.get(), target[0], target[1]);

    root->print();
    cout << endl;

    for (const auto& elem : result)
    {
    	cout << elem << endl;
    }
    return 0;
}
