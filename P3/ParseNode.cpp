/*
 * ParseNode.cpp
 *
 *  Created on: Apr 2, 2017
 *      Author: gerardryan
 */
#include <vector>
#include <regex>
#include <string>
#include <stack>
#include <map>

#include "ParseNode.h"
#include "polylex.h"

using namespace std;

extern int currentLine;
bool firstStatement = true;

static stack<Token *> tokenQueue = stack<Token *>();

extern map<string,bool> *IdentifierMap;


Token *GetToken(istream& in) {
    if(tokenQueue.size()> 0){
        Token *n = tokenQueue.top();
        tokenQueue.pop();
        return n;
    } else {
        return getToken(in);
    }
}


//For parse errors
void parseError(string s) {
    cout << "PARSE ERROR: " << currentLine << " " << s << endl;
    ++globalErrorCount;
}
//For Runtime Errors
void runtimeError( string s) {
    cout << "RUNTIME ERROR: " << currentLine << " " << s << endl;
    ++globalErrorCount;
}

//function to "putback" tokens
void PutBackToken(Token& t) {
    tokenQueue.push(&t);
}

Token *getToken(istream& in){
    enum State { START, INID, INSTRING, INICONST, INFCONST, INCOMMENT};

    State lexstate = START;
    string lexeme = "";
    
    while(true){
        char ch = in.get();
        
        if( in.eof() || in.bad() )
            break;
        
        if( ch == '\n' ) {
            currentLine++;
            lexstate = START;
            continue;
        }
        
        if(isspace(ch) && (lexstate == START)) continue;
        switch( lexstate ) {
            case START:
                if( isspace(ch) )
                    continue;
                if( isalpha(ch) ) {
                    lexstate = INID;
                    lexeme += ch;
                    continue;
                } else if( isdigit(ch) ) {
                    lexstate = INICONST;
                    lexeme += ch;
                    continue;
                } else if(ch == ';'){
                    return new Token(SC, ";");
                } else if(ch == '+'){
                    return new Token(PLUS, "+");
                } else if(ch == '*'){
                    return new Token(STAR, "*");
                } else if(ch == '['){
                    return new Token(LSQ, "[");
                } else if(ch == ']'){
                    return new Token(RSQ, "]");
                } else if(ch == '('){
                    return new Token(LPAREN, "(");
                } else if(ch == ')'){
                    return new Token(RPAREN, ")");
                } else if( ch == '#' ) {
                    lexstate = INCOMMENT;
                    continue;
                } else if( ch == '{' ) {
                    return new Token(LBR, "{");
                } else if( ch == '}' ) {
                    return new Token(RBR, "}");
                } else if (ch == ','){
                    return new Token(COMMA, ",");
                } else if( ch == '"' ) {
                    lexstate = INSTRING;
                    break;
                } else if( ch == '-' ) {
                    // maybe minus? maybe leading sign on a number?
                    if( isdigit(in.peek()) ) {
                        lexstate = INICONST;
                        break;
                    } else return new Token(MINUS, "-");
                } else {
                    parseError("Error parsing lexeme " + lexeme);
                    return new Token(ERR,lexeme);
                }
                break;
                
            case INID:
                if( !isalnum(ch) ) {
                    in.putback(ch);
                    if(lexeme == "set"){
                        return new Token(SET, lexeme);
                    } else if (lexeme == "print"){
                        return new Token(PRINT, lexeme);
                    } else
                        return new Token(ID, lexeme);
                }
                lexeme += ch;
                break;
                
            case INSTRING:
                if( ch == '"' ) {
                    return new Token(STRING, lexeme);
                }
                else if( ch == '\n' ) {
                    parseError("string must be in one line.");
                    return new Token(ERR, lexeme);
                }
                lexeme += ch;
                break;
                
            case INICONST:
                if( isdigit(ch) ) {
                    lexeme += ch;
                    continue;
                } else if(ch == '.'){
                    lexeme += ch;
                    if(isdigit(in.peek())){
                        lexstate = INFCONST;
                        continue;
                    } else {
                        parseError("Invalid float.");
                        return new Token(ERR, lexeme);
                    }
                } else {
                    in.putback(ch);
                    if(lexeme.length())
                        return new Token(ICONST, lexeme);
                }
                break;
                
            case INFCONST:
                if( isdigit(ch) ) {
                    lexeme += ch;
                    continue;
                } else {
                    in.putback(ch);
                    if(lexeme.length())
                        return new Token(FCONST, lexeme);
                }
                break;
                
            case INCOMMENT:
                if( ch == '\n' ) {
                    currentLine++;
                    lexstate = START;
                }
                continue;
                break;
                
        }
    }
    // handle getting DONE or ERR when not in start state
    if(in.eof()){
        if( lexstate == START ) return new Token(DONE, "Done");
        if( lexstate == INSTRING) return new Token(DONE, "Done");
        if( lexstate == INCOMMENT) return new Token(DONE, "Done");
    }
    
    return new Token(ERR, lexeme);
}


// Prog := Stmt | Stmt Prog
ParseNode *Prog(istream& in) {
    ParseNode *stmt = Stmt(in);
    
    if( stmt != 0 ){
        return new StatementList(stmt, Prog(in));
    } else if(currentLine == 0 && (firstStatement)) {
        firstStatement = false;
        parseError("Invalid Statement");
    }
    return 0;
}

// Stmt := Set ID Expr SC | PRINT Expr SC
ParseNode *Stmt(istream& in) {
    Token *cmd = GetToken(in);
    if( *cmd == SET ) {
        Token *idTok = GetToken(in);
        if( *idTok != ID ) {
            parseError("Identifier required after set");
            return 0;
        }
        ParseNode *exp = Expr(in);
        if( exp == 0 ) {
            parseError("expression required after id in set");
            return 0;
        }
        if( *GetToken(in) != SC ) {
            parseError("semicolon required");
            return 0;
        }
        
        return new SetStatement(idTok->getLexeme(), exp);
    }
    else if( *cmd == PRINT ) {
        ParseNode *exp = Expr(in);
        if( exp == 0 ) {
            parseError("expression required after id in print");
            return 0;
        }
        
        if( *GetToken(in) != SC ) {
            parseError("semicolon required");
            return 0;
        }
        
        return new PrintStatement(exp);
    }
    return 0;
}

// Expr := Term { (+|-) Expr }
ParseNode *Expr(istream& in) {
    ParseNode *t1 = Term(in);
    if( t1 == 0 ) return 0;
    
    
    Token *op = GetToken(in);
    if( *op != PLUS && *op != MINUS ) {
        PutBackToken(*op);
        return t1;
    }
    ParseNode *t2 = Expr(in);
    
    if( t2 == 0 ) {
        parseError("expression required after + or - operator");
        return 0;
    }
    
    // combine t1 and t2 together
    if( *op == PLUS )
        t1 = new PlusOp(t1, t2);
    else
        t1 = new MinusOp(t1, t2);
    
    return t1;
    
}
// Term := Primary { * Primary }
ParseNode *Term(istream& in) {
    ParseNode *p = Primary(in);
    Token *j = GetToken(in);
    if(*j == STAR){
        return new TimesOp(p, Term(in));
    }
    PutBackToken(*j);
    return p;
}

// Primary :=  ICONST | FCONST | STRING | ( Expr ) | Poly
ParseNode *Primary(istream& in) {
    ParseNode *t1 = 0;
    Token *tt1 = GetToken(in);
    Token *tt2;
    
    if(*tt1 == ICONST){
        t1 = new Iconst(stoi(tt1->getLexeme()));
    }else if(*tt1 == FCONST){
        t1 = new Fconst(stof(tt1->getLexeme()));
    }else if(*tt1 == STRING){
        t1 = new Sconst(tt1->getLexeme());
    }else if(*tt1 == LBR || *tt1 == ID){
        PutBackToken(*tt1);
        t1 = Poly(in);
    }else if(*tt1 == LPAREN){
        t1 = Expr(in);
        tt2 = GetToken(in);
        if(*tt2 != RPAREN){
            parseError("Parenthesis don't match");
            return 0;
        }
    }else if(*tt1 == ID){
        t1 = new Ident(tt1->getLexeme());
    } else {
        t1 = 0;
        PutBackToken(*tt1);
    }
    
    return t1;
}

// Poly := LCURLY Coeffs RCURLY { EvalAt } | ID { EvalAt }
ParseNode *Poly(istream& in) {
    // note EvalAt is optional
    Token *tk = GetToken(in);
    if(*tk == LBR){
        ParseNode *coeffs = 0;
        coeffs = Coeffs(in);
        Token *tk2 = GetToken(in);
        if(coeffs == 0){
            parseError("No coefficients were specified between brackets");
            return 0;
        }
        if(*tk2 == RBR){
            Token *tk2 = GetToken(in);
            if(*tk2 == LSQ){
                PutBackToken(*tk2);
                return new EvaluateAt(coeffs, EvalAt(in));
            } else {
                PutBackToken(*tk2);
                return coeffs;
            }
            return 0;
            
        }
        
        return 0;
        
    } else if (*tk == LSQ){
        PutBackToken(*tk);
        return EvalAt(in);
    } else if (*tk == ID){
        Token *tk2 = GetToken(in);
        if(*tk2 == LSQ){
            PutBackToken(*tk2);
            return new EvaluateAt(new Ident(tk->getLexeme()), EvalAt(in));
        }else if(*tk2 == RSQ){
            PutBackToken(*tk2);
        }
            PutBackToken(*tk2);
        return new Ident(tk->getLexeme());
    
    }
    return 0;
}
ParseNode *GetOneCoeff(Token& t){
    if( t == ICONST ) {
        return new Iconst(stoi(t.getLexeme()));
    } else if( t == FCONST ) {
        return new Fconst(stof(t.getLexeme()));
    }
    return 0;
}
// notice we don't need a separate rule for ICONST | FCONST
// this rule checks for a list of length at least one
ParseNode *Coeffs(istream& in) {
    vector<ParseNode *> coeffs;
    
    Token *t = GetToken(in);
    
    if (*t == COMMA) {
        parseError("No value provided before comma");
        return 0;
    }
    ParseNode *p = GetOneCoeff(*t);
    if( p == 0 )
        return 0;
    
    coeffs.push_back(p);
    
    while( true ) {
        t = GetToken(in);
        
        if( *t == COMMA ) {
            
            continue;
        } else if ( *t == RBR){
            PutBackToken(*t);
            return new Coefficients(coeffs);
        } else {
            p = GetOneCoeff(*t);
            if( p == 0 ) {
                parseError("Missing coefficient after comma");
                return 0;
            }
            coeffs.push_back(p);
        }
    }
    return new Coefficients(coeffs); // Coefficients class must take vector
}

// To evauluate the polynomials
ParseNode *EvalAt(istream& in) {
    Token *tk = GetToken(in);
    if(*tk == SC){
        PutBackToken(*tk);
        return 0;
    } else if(*tk == LSQ){
        ParseNode *n = Expr(in);
        Token *tk2 = GetToken(in);
        

        if(*tk2 != RSQ){
            parseError("Square braces don't match");
            return 0;
        }
        return n;
    }
    
    return 0;
}



