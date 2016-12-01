#include <vector>
#include <random>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <numeric>
#include <array>
#include <thread>
#include <type_traits>

#include "short_alloc.h"

using namespace std;

template <class T>
using DistSelector_t = conditional_t<is_integral<T>::value, uniform_int_distribution<>, uniform_real_distribution<>>;

template <class T, size_t N = 100, class Distribution = DistSelector_t<T>>
class FuckingRanomizer
{
    FuckingRanomizer(T _min, T _max) 
        : min(_min),
        max(_max),
        gen(random_device()()),
        dis(min, max),
        currProdPos(0),
        currConsPos(0)
    {
        prod = make_unique<thread>([&this] {
            while (this->currProdPos % N != this->currConsPos % N)
            {
                this->currProdPos %= N;
                this->currConsPos %= N;
                vals[this->currProdPos++] = dis(gen);
            }
        });
    }
    T getNext()
    {
        return T();
    }
private:
    T min;
    T max;
    T vals[N];
    size_t currProdPos;
    size_t currConsPos;
    mt19937 gen;
    Distribution dis;
    unique_ptr<thread> prod;
};

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
    double sum = accumulate(probs.begin(), probs.end(), 0.);
    for (size_t i = 1; i < probs.size(); i++)
    {
        probs[i] += probs[i - 1];
    }
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, sum);
    double rand = dis(gen);
    size_t targetPos = 0;
    for (; targetPos < probs.size(); targetPos++)
    {
        if (probs[targetPos] > rand)
        {
            break;
        }
    }
    return values[targetPos];
}

template <class T, size_t N = 200>
auto& arenaFor()
{
    static arena<sizeof(T) * N, alignof(T)> a{};
    return a;
}

class Node
{
public:
    virtual ~Node() = default;
    virtual int eval(int n, int xp, int xpp) = 0;
    virtual int size() = 0;
    virtual void print() = 0;
};

using NodePtr = Node*;

// 0-9
class Value : public Node
{
public:
    Value(int _data) : data(_data) {}
    ~Value() = default;
    virtual int eval(int n, int xp, int xpp) override { return data; }
    virtual int size() override { return 1; }
    virtual void print() override { cout << data << " "; }

    void* Value::operator new (size_t count)
    {
        return arenaFor<Value>().allocate<alignof(Value)>(count);
    }
    void Value::operator delete(void *ptr)
    {
        arenaFor<Value>().deallocate((char*)ptr, sizeof(Value));
    }
    static void resetArena()
    {
        arenaFor<Value>().reset();
    }
private:
    int data;
};


enum class VariableType
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
    virtual int eval(int n, int xp, int xpp) override { 
        switch (type)
        {
        case VariableType::N:
            return n;
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
    virtual int size() override { return 1; }
    virtual void print() override { cout << VarTypeToStr[type] << " "; }

    void* Variable::operator new (size_t count)
    {
        return arenaFor<Variable>().allocate<alignof(Variable)>(count);
    }
    void Variable::operator delete(void *ptr)
    {
        arenaFor<Variable>().deallocate((char*)ptr, sizeof(Variable));
    }
    static void resetArena()
    {
        arenaFor<Variable>().reset();
    }

private:
    VariableType type;
};

enum class OperationType
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
    virtual int eval(int n, int xp, int xpp) override {
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
    virtual int size() override { return 1 + nodeStg.first->size() + nodeStg.second->size(); }
    virtual void print() override { 
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
    static void resetArena()
    {
        arenaFor<Operation>().reset();
    }
private:
    OperationType operation;
    std::pair<NodePtr, NodePtr> nodeStg;
};


NodePtr generate_operations()
{
    NodePtr root;
    if (flip(0.7))
    {//добавить новое непосредственное значение
        switch (getRand<0, 1>())
        {
        case 0:
            root = new Value(getRand< 0, 9>());
            break;
        case 1:
            root = new Variable((VariableType)(randFrom<3>({ 0,1,2 }, {50, 0, 0})));
            break;
        }
    }
    else
    {//добавить новый узел вычислений
        auto pair = make_pair(generate_operations(), generate_operations());
        root = new Operation((OperationType)getRand<0, 2>(), pair);
    }
    return root;
}

template <size_t N>
array<int, N> calculate(NodePtr operation_tree, int xpp, int xp)
{
    int count = N;
    array<int, N> res_seq;
    res_seq[0] = xpp;
    res_seq[1] = xp;
    for (int i = 0; i < count; i++)
    {
        res_seq[i] = operation_tree->eval(i+1, 0, 0);
    }
    return res_seq;
}


int main()
{
    using T = conditional<is_integral<float>::value, uniform_int_distribution<>, uniform_real_distribution<>>::type;
    T v;
    //map<int, int> fre;
    //for (int i = 0; i < 1000000; i++)
    //{
    //    int rn = randFrom({ 1,2,3 }, { 25,50,25 });
    //    fre[rn]++;
    //}
    constexpr int sz = 3;
    constexpr array<int, sz> target{ -9,-2,-3 };
    array<int, sz> result;
    unique_ptr<Node> root;
    while (true)
    {
        Value::resetArena();
        Variable::resetArena();
        Operation::resetArena();
        root.reset(generate_operations());
        result = calculate<sz>(root.get(), target[0], target[1]);
        if (target == result)
        {
            break;
        }
    }

    root->print();
    cout << endl;

    for (const auto& elem : result)
    {
    	cout << elem << endl;
    }
    return 0;
}
