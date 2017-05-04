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
#include <cmath>

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
extern void runtimeError(string s);

// objects in the language have one of these types
enum Type {
	INTEGERVAL,
	FLOATVAL,
	STRINGVAL,
    POLYVAL,
	UNKNOWNVAL,
};


// this class will be used in the future to hold results of evaluations
class Value {
	int	i;
	float f;
	string s;
	Type	t;
    vector<Value *> p;
public:
	Value(int i) : i(i), f(0), t(INTEGERVAL) {}
	Value(float f) : i(0), f(f), t(FLOATVAL) {}
	Value(string s) : i(0), f(0), s(s), t(STRINGVAL) {}
    Value(vector<Value *> p) : i(0), f(0), p(p), t(POLYVAL) {}
    Value() : t(UNKNOWNVAL) {}

    Value operator+(const Value& op) const {
        if( t == INTEGERVAL ) {
            if( op.t == INTEGERVAL )
                return Value(i + op.i);
            else if( op.t == FLOATVAL )
                return Value((float)i + op.f);
        }else if( t == FLOATVAL ) {
            if( op.t == INTEGERVAL )
                return Value(f + (float)op.i);
            else if( op.t == FLOATVAL )
                return Value(f + op.f);
        }else if( t == STRINGVAL ) {
            if( op.t == STRINGVAL )
                return Value(s + op.s);
        }else if (t == POLYVAL){
            if( op.t == POLYVAL){
                vector<Value *> *p2 =  new vector<Value *>();
                for(int i = (int)p.size(); i > 0; i--){
                    if(i <= op.p.size() && i <= p.size()){
                        p2->push_back(new Value((*p[i]) + (*op.p[i])));
                    } else{
                        if(p.size() < i){
                            p2->push_back(new Value(*op.p[i]));
                        } else {
                            p2->push_back(new Value(*p[i]));
                        }
                    }
                }
                
                return Value(*p2);
            } else if (op.t == INTEGERVAL){
                vector<Value *> *p2 = new vector<Value *>();
                for(int i = (int)p.size(); i > 0; i--){
                    p2->push_back(new Value((*p[i]) + op.i));
                }
                
                return Value(*p2);
            }
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
                return Value(f * (float)op.i);
            }
        } else if( t == STRINGVAL){
            if(op.t == INTEGERVAL){
                string multistring = ""; // consider initializing with value of s
                for(unsigned i = 0; i < op.i; i ++){
                    multistring += s;
                }
                return Value(multistring);
            }
        }
        return Value();
    }
    
    
    Type GetType() { return t; }
    int GetIntValue(){return i;}
    float GetFloatValue(){return f;}
    string GetStringValue(){return s;}
    vector<Value *> GetPolyValue() {return p; }
    
    friend ostream &operator<<( ostream &output, const Value &v ) {
        if(v.t == INTEGERVAL){
            output << v.i << endl;
        } else if(v.t == STRINGVAL){
            output << v.s << endl;
        }else if(v.t == FLOATVAL){
            output << v.f << endl;
        }else if(v.t == POLYVAL){
            
            output << "{ ";
            for(int i = 0; i < v.p.size(); i++){
                output << (*v.p[i]).GetIntValue();
                if(i != v.p.size()-1){
                    output << ", ";
                }
            }
            output << " }\n";
        }
        return output;
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
//	virtual Type GetType() { return UNKNOWNVAL; }
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
    virtual Type GetType(){return UNKNOWNVAL;}
};
extern void runtimeError(string s);

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
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        if( op1.GetType() == UNKNOWNVAL ) {
            runtimeError("Unknown val in set statement.");
        }
        symb[id] = op1;
        return op1;
    }
    
};

// a PrintStatement represents the idea of printing the value of the Expr pointed to by the left node
class PrintStatement : public ParseNode {
public:
	PrintStatement(ParseNode* exp) : ParseNode(exp) {}
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        if( op1.GetType() == UNKNOWNVAL ) {
            runtimeError("Unknown val in set statement.");
        }
        cout << op1;
        return op1;
    }
};

// represents adding
class PlusOp : public ParseNode {
public:
	PlusOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
    Value Eval(map<string,Value>& symb) {
        Value op1 = leftNode()->Eval(symb);
        Value op2 = rightNode()->Eval(symb);
        Value sum = op1 + op2;
        if( sum.GetType() == UNKNOWNVAL ) {
            runtimeError("type mismatch in add");
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
            runtimeError("type mismatch in subtract");
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
            runtimeError("type mismatch in multiply");
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
    
    Value Eval(map<string,Value>& symb) {
        vector<Value *> l =  vector<Value *>();
        vector<ParseNode *>::iterator It;
        int i =0;
        ParseNode* thing;
        for(It = coefficients.begin(); It != coefficients.end(); It++)
        {
            thing = *It;
            
            
            l.push_back(new Value(thing->Eval(symb)));
            
            i++;
        }
        return Value(l);
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
            runtimeError("identifier " + id + " used before set");
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
        
        if( op1.GetType() != POLYVAL ) {
            runtimeError( "type mismatch in EvaluateAt");
        }
        
        if(op2.GetType()!=FLOATVAL || op2.GetType()!=INTEGERVAL){
            runtimeError( "type mismatch");
            return Value();
        }
        
        vector<Value *> temp = op1.GetPolyValue();
        
        bool isFloat = false;
        if(op2.GetType() == FLOATVAL){
            isFloat = true;
        }
        for(int i = 0; i < temp.size(); i++){
            if(isFloat) break;
            if(temp[i]->GetType() == FLOATVAL){
                isFloat = true;
            }
        }
        
        
        if(isFloat){
            float j = (float) temp.size() - 1;
            float val;
            float val2;
            float sum = 0.0;
            for(int i = 0; i < temp.size(); i++){
                
                if(temp[i]->GetType()== FLOATVAL){
                    val = temp[i]->GetFloatValue();
                } else {
                    val = (float) temp[i]->GetIntValue();
                }
                    
                if(op2.GetType() == FLOATVAL){
                    val2 = op2.GetFloatValue();
                }else {
                    val2 = (float) op2.GetIntValue();
                }
                sum += pow(val2, j) * val;
                j--;
            }
            return Value(sum);
        }else {
            int j = (int) temp.size() - 1;
            int val;
            int val2;
            int sum = 0;
            for(int i = 0; i < temp.size(); i++){
                val = temp[i]->GetIntValue();
                val2 = op2.GetIntValue();
                sum += pow(val2, j) * val;
                j--;
            }
            return Value(sum);
        }
        
        
        return op1;
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
