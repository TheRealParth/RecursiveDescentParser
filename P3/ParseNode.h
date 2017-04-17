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

using std::istream;
using std::cout;
using std::endl;
using std::string;
using std::vector;
#include "polylex.h"

extern int globalErrorCount;

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

	Type GetType() { return t; }
	int GetIntValue();
	float GetFloatValue();
	string GetStringValue();

};

// every node in the parse tree is going to be a subclass of this node
class ParseNode {
	ParseNode	*left;
	ParseNode	*right;
public:
	ParseNode(ParseNode *left = 0, ParseNode *right = 0) : left(left), right(right) {}
	virtual ~ParseNode() {}

	virtual Type GetType() { return UNKNOWNVAL; }
};

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
};

// represents subtracting
class MinusOp : public ParseNode {
public:
    MinusOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
};

// represents multiplying the two child expressions
class TimesOp : public ParseNode {
public:
	TimesOp(ParseNode *l, ParseNode *r) : ParseNode(l,r) {}
};


// a representation of a list of coefficients must be developed
class Coefficients : public ParseNode {
    vector<ParseNode *> coefficients;
public:
    Coefficients(ParseNode *l, ParseNode *r, vector<ParseNode *> &coeff) : ParseNode(l, r){
        coefficients = coeff;
    }
    Coefficients(vector<ParseNode *> &coeff) : ParseNode(){
        coefficients = coeff;
    }
};

// represents evaluating a polynomial
class EvaluateAt : public ParseNode {
public:
    EvaluateAt(ParseNode *idorcoeff, ParseNode *index) : ParseNode(idorcoeff,index) {
        
    }
};

// leaves of the parse tree
// notice that the parent constructors take no arguments
// that means this is a leaf
class Iconst : public ParseNode {
	int	iValue;
public:
	Iconst(int iValue) : iValue(iValue), ParseNode() {}
	Type GetType() { return INTEGERVAL; }
};

class Fconst : public ParseNode {
	float	fValue;
public:
	Fconst(float fValue) : fValue(fValue), ParseNode() {}
	Type GetType() { return FLOATVAL; }
};

class Sconst : public ParseNode {
	string	sValue;
public:
	Sconst(string sValue) : sValue(sValue), ParseNode() {}
	Type GetType() { return STRINGVAL; }
};


class Ident : public ParseNode {
	string	id;
public:
	Ident(string id) : id(id), ParseNode() {}
    Type GetType() { return UNKNOWNVAL; }; // not known until run time!
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
