/*
 * ParseNode.h
 *
 *  Created on: Apr 2, 2017
 *      Author: gerardryan
 */

#ifndef PARSENODE_H_
#define PARSENODE_H_

#include <iostream>
#include <string>
#include <map>

using std::istream;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::ostream;

#include "polylex.h"

extern int globalErrorCount;
extern int currentLine;
extern map<string,bool> *IdentifierMap;


// objects in the language have one of these types
enum Type {
	INTEGERVAL,
	FLOATVAL,
	STRINGVAL,
	UNKNOWNVAL,
};


// this class will be used in the future to hold results of evaluations
class Value {
	int	i;
	float f;
	string s;
	Type	t;
public:
	Value(int i) : i(i), f(0), t(INTEGERVAL) {}
	Value(float f) : i(0), f(f), t(FLOATVAL) {}
	Value(string s) : i(0), f(0), s(s), t(STRINGVAL) {}
    Value() : t(UNKNOWNVAL) {}

    Value operator+(const Value& op) const {
        if( t == INTEGERVAL ) {
            if( op.t == INTEGERVAL )
                return Value( i + op.i );
            else if( op.t == FLOATVAL )
                return Value( i + op.f );
        }else if( t == FLOATVAL ) {
            if( op.t == INTEGERVAL )
                return Value( f + op.i );
            else if( op.t == FLOATVAL )
                return Value( f + op.f );
        }else if( t == STRINGVAL ) {
            if( op.t == STRINGVAL )
                return Value( s + op.s );
        }
        return Value();
    }
    
    Value operator-(const Value& op) const {
        if( t == INTEGERVAL){
            if(op.t == INTEGERVAL)
                return Value(i - op.i);
            else if (op.t == FLOATVAL)
                return Value((float)i - op.f);
        } else if( t == FLOATVAL ){
            if(op.t == FLOATVAL)
                return Value(f - op.f);
            else if(op.t == INTEGERVAL)
                return Value(f - (float)op.i);
        }
        return Value();
    }
    
    Value operator*(const Value& op) const {
        if( t == INTEGERVAL){
            if(op.t == FLOATVAL){
                return Value((float)i * op.f);
            } else if(op.t == INTEGERVAL){
                return Value(i * op.i);
            } else if(op.t == STRINGVAL){
                if(op.t == INTEGERVAL){
                    string multistring = ""; // consider initializing with value of s
                    for(int j = 0; j < i; j++){
                        multistring += op.s;
                    }
                    return Value(multistring);
                }
            }
        } else if( t == FLOATVAL){
            if(op.t == FLOATVAL){
                return Value(f * op.f);
            } else if (op.t == INTEGERVAL){
                return Value(f * (int)op.i);
            }
        } else if( t == STRINGVAL){
            if(op.t == INTEGERVAL){
                string multistring = ""; // consider initializing with value of s
                for(int i = 0; i < op.i; i ++){
                    multistring += s;
                }
                return Value(multistring);
            }
        }
        return Value();
    }
    
    Type GetType() { return t; }
    int GetIntValue();
    float GetFloatValue();
    string GetStringValue();
    
    friend ostream& operator<<(ostream& os, const Value& t) {
        if(t.t == INTEGERVAL){
            os << t.i;
        }else if(t.t == STRINGVAL){
            os << t.s;
        } else if (t.t == FLOATVAL){
            os << t.f;
        }
        return os;
    }


};

extern map<string, Value> *Symb;

// every node in the parse tree is going to be a subclass of this node
class ParseNode {
	ParseNode	*left;
	ParseNode	*right;
    int whichLine;
public:
	ParseNode(ParseNode *left = 0, ParseNode *right = 0) : left(left), right(right) {
        whichLine = currentLine;
    }
	virtual ~ParseNode() {}
	virtual Type GetType() { return UNKNOWNVAL; }
    virtual int getLine() { return whichLine; }
    virtual void RunStaticChecks(map<string,bool>& idMap) {
        if( left )
            left->RunStaticChecks(idMap);
        if( right )
            right->RunStaticChecks(idMap);
    }
    virtual Value
    Eval(map<string,Value>& symb) {
        if( left ) left->Eval(symb);
        if( right ) right->Eval(symb);
        return Value();
    }
    ParseNode *rightNode() {
        return right;
    };
    
    ParseNode *leftNode() {
        return left;
    };
    
};
extern void runtimeError(ParseNode* i, string s);

// a list of statements is represented by a statement to the left, and a list of statments to the right
class StatementList : public ParseNode {
public:
	StatementList(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
};

// a SetStatement represents the idea of setting id to the value of the Expr pointed to by the left node
class SetStatement : public ParseNode {
	string id;
public:
	SetStatement(string id, ParseNode* exp) : id(id), ParseNode(exp) {}
    void RunStaticChecks(map<string,bool>& idMap)
    {
        idMap[id] = true;
    }
};

// a PrintStatement represents the idea of printing the value of the Expr pointed to by the left node
class PrintStatement : public ParseNode {
public:
	PrintStatement(ParseNode* exp) : ParseNode(exp) {}
    
};

// represents adding
class PlusOp : public ParseNode {
public:
	PlusOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        Value op2 = rightNode()->Eval(symb);
        Value sum = op1 + op2;
        cout << sum << endl;
        if( sum.GetType() == UNKNOWNVAL ) {
            runtimeError(this, "type mismatch in add");
        }
        return sum;
    }
};

// represents subtracting
class MinusOp : public ParseNode {
public:
    MinusOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        Value op2 = rightNode()->Eval(symb);
        Value sum = op1 - op2;
        if( sum.GetType() == UNKNOWNVAL ) {
            runtimeError(this, "type mismatch in subtract");
        }
        return sum;
    }
};

// represents multiplying the two child expressions
class TimesOp : public ParseNode {
public:
	TimesOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        Value op2 = rightNode()->Eval(symb);
        Value product = op1 * op2;
        if( product.GetType() == UNKNOWNVAL ) {
            runtimeError(this, "type mismatch in multiply");
        }
        return product;
    }
};


// a representation of a list of coefficients must be developed
class Coefficients : public ParseNode {
    vector<ParseNode *> coefficients;
public:
    Coefficients(vector<ParseNode *> &coeff) : ParseNode(){
        coefficients = coeff;
    }
};



// leaves of the parse tree
// notice that the parent constructors take no arguments
// that means this is a leaf
class Iconst : public ParseNode {
	int	iValue;
public:
	Iconst(int iValue) : iValue(iValue), ParseNode() {}
    int GetIntValue(){
        return iValue;
    }
    Value Eval(map<string,Value>& symb) {
        return Value(iValue);
    }
    Type GetType() { return INTEGERVAL; }
};

class Fconst : public ParseNode {
	float	fValue;
public:
	Fconst(float fValue) : fValue(fValue), ParseNode() {}
    float GetFloatValue(){ return fValue;}
    Value Eval(map<string,Value>& symb) {
        return Value(fValue);
    }
	Type GetType() { return FLOATVAL; }
};

class Sconst : public ParseNode {
	string	sValue;
public:
	Sconst(string sValue) : sValue(sValue), ParseNode() {}
    string GetStringValue(){ return sValue; }
    Value Eval(map<string,Value>& symb) {
        return Value(sValue);
    }
	Type GetType() { return STRINGVAL; }
};


class Ident : public ParseNode {
	string	id;
    Type t;
public:
	Ident(string id) : id(id), t(UNKNOWNVAL), ParseNode() {}
    void RunStaticChecks(map<string,bool>& idMap) {
        if( idMap[id] == false ) {
            runtimeError(this, "identifier " + id + " used before set");
            ++globalErrorCount;
        }
    }
    Value Eval(map<string,Value>& symb) {
        t = symb[id].GetType();
        return symb[id];
    }
    Type GetType() { return t; }; // not known until run time!
};

// represents evaluating a polynomial
class EvaluateAt : public ParseNode {
public:
    EvaluateAt(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}

    
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        Value op2 = rightNode()->Eval(symb);
        
        Value sum = op1 - op2;
        if( sum.GetType() == UNKNOWNVAL ) {
            runtimeError(this, "type mismatch in subtract");
        }
        return sum;
    }
};


extern ParseNode *Prog(istream& in);
extern ParseNode *Stmt(istream& in);
extern ParseNode *Expr(istream& in);
extern ParseNode *Term(istream& in);
extern ParseNode *Primary(istream& in);
extern ParseNode *Poly(istream& in);
extern ParseNode *Coeffs(istream& in);
extern ParseNode *EvalAt(istream& in);


#endif /* PARSENODE_H_ */
