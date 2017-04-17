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
#include "ParseNode.h"
#include "polylex.h"


using namespace std;
// We want to use our getToken routine unchanged... BUT we want to have the ability
// to push back a token if we read one too many; this is our "lookahead"
// so we implement a "wrapper" around getToken

static bool pushedBack = false;
static stack<Token> tokenQueue = stack<Token>();

Token GetToken(istream& in) {
    if(tokenQueue.size()> 0){
        Token &n = tokenQueue.top();
        tokenQueue.pop();
        return n;
    } else {
        return getToken(in);
    }
}

// handy function to print out errors
void error(string s, int errType = 0) {
    if(errType == 0) cout << "PARSE ERROR: ";
    if(errType == 1) cout << "RUNTIME ERROR: ";
    cout << currentLine << " " << s << endl;
    
    ++globalErrorCount;
}
void PutBackToken(Token& t) {
    tokenQueue.push(t);
}

Token getToken(istream& in){
    enum State { START, INID, INSTRING, INICONST, INFCONST, INCOMMENT};
    
    State lexstate = START;
    std::string lexeme = "";
    
    while(true){
        char ch = in.get();
        
        if( in.eof() || in.bad() )
            break;
        
        if( ch == '\n'  ) {
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
                    return Token(SC, ";");
                } else if(ch == '+'){
                    return Token(PLUS, "+");
                } else if(ch == '*'){
                    return Token(STAR, "*");
                } else if(ch == '['){
                    return Token(LSQ, "[");
                } else if(ch == ']'){
                    return Token(RSQ, "]");
                } else if(ch == '('){
                    return Token(LPAREN, "(");
                } else if(ch == ')'){
                    return Token(RPAREN, ")");
                } else if( ch == '#' ) {
                    lexstate = INCOMMENT;
                    continue;
                } else if( ch == '{' ) {
                    return Token(LBR, "{");
                } else if( ch == '}' ) {
                    return Token(RBR, "}");
                } else if (ch == ','){
                    return Token(COMMA, ",");
                } else if( ch == '"' ) {
                    lexstate = INSTRING;
                    break;
                } else if( ch == '-' ) {
                    // maybe minus? maybe leading sign on a number?
                    if( isdigit(in.peek()) ) {
                        lexstate = INICONST;
                        break;
                    } else return Token(MINUS, "-");
                } else {
                    error("Error parsing lexeme " + lexeme);
                    return Token(ERR,lexeme);
                }
                break;
                
            case INID:
                if( !isalnum(ch) ) {
                    in.putback(ch);
                    if(lexeme == "set"){
                        return Token(SET, lexeme);
                    } else if (lexeme == "print"){
                        return Token(PRINT, lexeme);
                    } else
                        return Token(ID, lexeme);
                }
                lexeme += ch;
                break;
                
            case INSTRING:
                if( ch == '"' ) {
                    return Token(STRING, lexeme);
                }
                else if( ch == '\n' ) {
                    error("string must be in one line.");
                    return Token(ERR, lexeme);
                }
                lexeme += ch;
                break;
                
            case INICONST:
                if( isdigit(ch) ) {
                    lexeme += ch;
                    continue;
                } else {
                    in.putback(ch);
                    if(lexeme.length())
                    return Token(ICONST, lexeme);
                }
                break;
                
            case INFCONST:
                if( isdigit(ch) ) {
                    lexeme += ch;
                    continue;
                } else {
                    in.putback(ch);
                    if(lexeme.length())
                    return Token(FCONST, lexeme);
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
        if( lexstate == START ) return Token(DONE, "Done");
        if( lexstate == INSTRING) return Token(DONE, "Done");
        if( lexstate == INCOMMENT) return Token(DONE, "Done");
    }
    
    return Token(ERR, lexeme);
}


// Prog := Stmt | Stmt Prog
ParseNode *Prog(istream& in) {
    ParseNode *stmt = Stmt(in);

    if( stmt != 0 )
        return new StatementList(stmt, Prog(in));
    
    return 0;
}

// Stmt := Set ID Expr SC | PRINT Expr SC
ParseNode *Stmt(istream& in) {
    Token cmd = GetToken(in);
    
    if( cmd == SET ) {
        Token idTok = GetToken(in);
        if( idTok != ID ) {
            error("Identifier required after set");
            return 0;
        }
        ParseNode *exp = Expr(in);
        if( exp == 0 ) {
            error("expression required after id in set");
            return 0;
        }
        if( GetToken(in) != SC ) {
            error("semicolon required");
            return 0;
        }
        
        return new SetStatement(idTok.getLexeme(), exp);
    }
    else if( cmd == PRINT ) {
        ParseNode *exp = Expr(in);
        if( exp == 0 ) {
            error("expression required after id in print");
            return 0;
        }
        if( GetToken(in) != SC ) {
            error("semicolon required");
            return 0;
        }
        
        return new PrintStatement(exp);
    }
    else
        PutBackToken(cmd);
    return 0;
}

ParseNode *Expr(istream& in) {
    ParseNode *t1 = Term(in);
    
    if( t1 == 0 ) return 0;
    
    
    Token op = GetToken(in);
    if( op != PLUS && op != MINUS ) {
        PutBackToken(op);
        return t1;
    }
    
    ParseNode *t2 = Expr(in);
    
    if( t2 == 0 ) {
        error("expression required after + or - operator");
        return 0;
    }
    
    // combine t1 and t2 together
    if( op == PLUS )
        t1 = new PlusOp(t1, t2);
    else
        t1 = new MinusOp(t1, t2);
    
    return t1;
    
}

ParseNode *Term(istream& in) {
    ParseNode *p = Primary(in);
    Token j = GetToken(in);
    if(j == STAR){
        return new TimesOp(p, Term(in));
    }
    
    PutBackToken(j);
    return p;
}

// Primary :=  ICONST | FCONST | STRING | ( Expr ) | Poly
ParseNode *Primary(istream& in) {
    ParseNode *t1 = 0;
    Token tt1 = GetToken(in);
    Token tt2;
    if(tt1 == ICONST){
        cout << "1738 : " << tt1.getLexeme() << endl;
        t1 = new Iconst(std::stoi(tt1.getLexeme()));
    }else if(tt1 == FCONST){
        t1 = new Fconst(std::stof(tt1.getLexeme()));
    }else if(tt1 == STRING){
        t1 = new Sconst(tt1.getLexeme());
    }else if(tt1 == LBR){
        t1 = Poly(in);
        tt2 = GetToken(in);
        if(tt2 != RBR){
            error("Curly braces don't match");
            
            return 0;
        } else {
            return t1;
        }
    }else if(tt1 == LPAREN){
        t1 = Expr(in);
        tt2 = GetToken(in);
        if(tt2 != RPAREN){
            error("Parenthesis don't match");
            return 0;
        }
    }else if(tt1 == LSQ){
        t1 = Expr(in);
        tt2 = GetToken(in);
        if(tt2 != RSQ){
            error("Square braces don't match");
            return 0;
        }
    }
    return t1;
}

// Poly := LCURLY Coeffs RCURLY { EvalAt } | ID { EvalAt }
ParseNode *Poly(istream& in) {
    // note EvalAt is optional
    
    ParseNode *coeffs = 0;
    coeffs = Coeffs(in);
    
    if(coeffs == 0){
        error("No coefficients were specified between brackets");
        return 0;
    }
    return new EvaluateAt(coeffs, EvalAt(in));
}
ParseNode *GetOneCoeff(Token& t){
    if( t == ICONST ) {
        return new Iconst(std::stoi(t.getLexeme()));
    } else if( t == FCONST ) {
        return new Fconst(std::stof(t.getLexeme()));
    }
    return 0;
}
// notice we don't need a separate rule for ICONST | FCONST
// this rule checks for a list of length at least one
ParseNode *Coeffs(istream& in) {
    std::vector<ParseNode *> coeffs;
    
    Token t = GetToken(in);
    
    if (t == COMMA) {
        error("No value provided before comma");
    }
    
    ParseNode *p = GetOneCoeff(t);
//    cout << " TOKEN TYPE : " << t.getType() << " |   TOKEN LEXEME: " << t.getLexeme() << endl;
    if( p == 0 )
        return 0;

    coeffs.push_back(p);
    
    while( true ) {
        t = GetToken(in);
        
        if( t == COMMA ) {
            continue;
        } else if ( t == RBR){
            PutBackToken(t);
            return new Coefficients(coeffs);
        } else {
            p = GetOneCoeff(t);
            if( p == 0 ) {
                error("Missing coefficient after comma");
                return 0;
            }
            coeffs.push_back(p);
        }
    }
    return new Coefficients(coeffs); // Coefficients class must take vector
}

// To evauluate the polynomials
ParseNode *EvalAt(istream& in) {
    ParseNode *p1 = 0;
    Token tok1 = GetToken(in);
    Token tok2;
    if(tok1==LSQ){
        p1 = Expr(in);
        tok2 = GetToken(in);
        if(tok2 == RSQ){
            return p1;
        } else {
            error("No closing square bracket");
            return 0;
        }
    }
    
    return 0;
}
