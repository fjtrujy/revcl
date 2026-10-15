// First reconstruction of previously untouched VUC integer binary folding.
// Original cases 0..13 are visible directly in the ELF control flow.
// This remains an exploratory C++03 template until both specializations match.
namespace IR {
struct Operator_Data {
    unsigned virtual_table;
    unsigned operation;
};
}

template<class T>
bool ReduceBinOp(IR::Operator_Data *op, const T &lhs, const T &rhs, T &out)
{
    unsigned operation=op->operation;
    if(operation==13) out=lhs+rhs;
    else if(operation==12) out=lhs-rhs;
    else if(operation==11) out=lhs*rhs;
    else if(operation==10) out=lhs/rhs;
    else if(operation==7) out=lhs==rhs;
    else if(operation==6) out=lhs!=rhs;
    else if(operation==5) out=lhs>rhs;
    else if(operation==4) out=lhs<rhs;
    else if(operation==3) out=lhs>=rhs;
    else if(operation==2) out=lhs<=rhs;
    else if(operation==1) out=lhs&&rhs;
    else if(operation==0) out=lhs||rhs;
    else if(operation==9) out=lhs&rhs;
    else if(operation==8) out=lhs|rhs;
    else return true;
    return true;
}

template bool ReduceBinOp<unsigned>(IR::Operator_Data *, const unsigned &, const unsigned &, unsigned &);
template bool ReduceBinOp<int>(IR::Operator_Data *, const int &, const int &, int &);
