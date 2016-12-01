#include <vector>
#include <random>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <numeric>

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
    return dis(gen) < p ? true : false;
}

int randFrom( const vector<int> &values, vector<double> probs)
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

class Node
{
public:
    virtual ~Node() = default;
    virtual int eval(int n, int xp, int xpp) = 0;
    virtual int size() = 0;
    virtual void print() = 0;
};

using NodePtr = Node*;
using NodeStorage = std::pair<NodePtr,NodePtr>;

// 0-9
class Value : public Node
{
public:
    Value(int _data) : data(_data) {}
    ~Value() = default;
    virtual int eval(int n, int xp, int xpp) override { return data; }
    virtual int size() override { return 1; }
    virtual void print() override { cout << data << " "; }
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
    Operation(OperationType _operation, const NodeStorage &_nodeStg) : operation(_operation), nodeStg(_nodeStg) {}
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
private:
    OperationType operation;
    NodeStorage nodeStg;
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
            root = new Variable((VariableType)(randFrom({ 0,1,2 }, {50, 25, 25})));
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

vector<int> calculate(NodePtr operation_tree, int count, int xpp, int xp)
{
    vector<int> res_seq(count);
    res_seq[0] = xpp;
    res_seq[1] = xp;
    for (int i = 2; i < count; i++)
    {
        res_seq[i] = operation_tree->eval(i+1, res_seq[i-1], res_seq[i-2]);
    }
    return res_seq;
}


int main()
{
    //map<int, int> fre;
    //for (int i = 0; i < 1000000; i++)
    //{
    //    int rn = randFrom({ 1,2,3 }, { 25,50,25 });
    //    fre[rn]++;
    //}

    vector<int> target{ 1, 4, 8, 16, 20};
    vector<int> result;
    unique_ptr<Node> root;
    while (true)
    {
        root.reset(generate_operations());
        result = calculate(root.get(), target.size(), target[0], target[1]);
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
