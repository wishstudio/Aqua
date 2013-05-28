#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stack>
#include <string>
#include <unordered_map>

#define ARRAYLENGTH(x) (sizeof(x) / sizeof(x[0]))

char ch;
enum TokenType
{
	/* Binop */
	tkAdd,
	tkSub,
	tkMul,
	tkDiv,
	tkRem,
	tkBitAnd,
	tkBitOr,
	tkBinArith = tkBitOr,

	tkLT,
	tkLE,
	tkGT,
	tkGE,
	tkEQ,
	tkNEQ,
	tkBinComparison = tkNEQ,

	tkAnd,
	tkOr,
	tkBinLogic = tkOr,

	tkBinOp = tkBinLogic,

	/* Unary op */
	tkNot,
	tkNeg,
	tkBitNot,
	tkAddressOf,
	tkDeref,
	tkUnaryOp = tkDeref,

	/* Other symbols */
	tkEOF,
	tkNum,
	tkIdent,
	tkString,
	tkComma,
	tkAssign,
	tkArrow,
	tkColon,
	tkDoubleColon,
	tkSemicolon,
	tkPLeft,
	tkPRight,
	tkSLeft,
	tkSRight,
	tkBLeft,
	tkBRight,
	tkArray,

	/* Reserved identifiers */
	tkThis,
	tkNull,
	tkFunction,
	tkOperator,
	tkNew,
	tkNewArray,
	tkIf,
	tkElse,
	tkFor,
	tkWhile,
	tkDo,
	tkReturn,
	tkTry,
	tkCatch,
	tkThrow,

	/* Pseudo-token for AST ops */
	tkTypeCast,
	tkArrayElement,
	tkList,
	tkMultipleNode,
	tkDefinition,
	tkCall,
	tkField,
} tt;
std::string ti;

/* Rollback save */
bool rollback = false;
TokenType rb_tt;
std::string rb_ti;

int binop_priority[] =
{
	10, 10, /* Add, Sub */
	11, 11, 11, /* Mul, Div, Rem */
	6, 6, /* BitAnd, BitOr */
	5, 5, 5, 5, 5, 5, /* Comparison */
	3, /* And */
	4, /* Or */
};

/* AST Nodes */
struct Node
{
	TokenType op;
};

struct Int32Node: public Node
{
	int intvalue;
};

struct StringNode: public Node
{
	std::string stringvalue;
};

struct IdentNode: public Node
{
	std::string ident;
};

struct FieldNode: public Node
{
	Node *object;
	std::string field;
};

struct CallNode: public Node
{
	Node *method;
	std::vector<Node *> parameters;
};

struct NewNode: public Node
{
	std::string type;
	std::vector<Node *> parameters;
};

struct UnaryNode: public Node
{
	Node *operand;
};

struct BinNode: public Node
{
	Node *left, *right;
};

struct ListNode: public Node
{
	std::vector<Node *> values;
};

struct MultipleNode: public Node
{
	std::vector<Node *> nodes;
};

struct DefinitionNode: public Node
{
	std::string type;
	std::vector<std::pair<std::string, Node *>> variableList;
};

struct TypeCastNode: public Node
{
	std::string type;
	Node *operand;
};

struct IfNode: public Node
{
	Node *condition;
	Node *trueBody, *falseBody;
};

struct WhileNode: public Node /* Also used for do-while */
{
	Node *condition;
	Node *body;
};

struct ForNode: public Node
{
	Node *init, *condition, *update;
	Node *body;
};

struct ReturnNode: public Node
{
	Node *result;
};

struct ThrowNode: public Node
{
	Node *exception;
};

struct Field
{
	int modifier;
	std::string name;
	std::string type;
};

struct Method
{
	int modifier;
	std::string name;
	std::string type;
	std::vector<std::string> paramTypes;
	std::vector<std::string> paramNames;
	std::vector<std::string> returnTypes;
	std::vector<std::string> returnNames;
	int cc;
	std::string libraryName, originalName;
	Node *body;
};

struct Class
{
	std::string name, baseName;
	std::vector<Method *> methods_vector;
	std::vector<Field *> fields_vector;
	std::unordered_map<std::string, Method *> methods;
	std::unordered_map<std::string, Field *> fields;
};

std::vector<Class *> classes_vector;
std::unordered_map<std::string, Class *> classes;
std::vector<std::string> code;
std::string padding;

struct ReservedIdentifier
{
	std::string name;
	TokenType op;
};
const ReservedIdentifier reservedIdentifiers[] = 
{
	{"this", tkThis},
	{"null", tkNull},
	{"function", tkFunction},
	{"operator", tkOperator},
	{"new", tkNew},
	{"if", tkIf},
	{"else", tkElse},
	{"for", tkFor},
	{"while", tkWhile},
	{"do", tkDo},
	{"return", tkReturn},
	{"try", tkTry},
	{"catch", tkCatch},
	{"throw", tkThrow},
};

void getCh()
{
	ch = getchar();
}

void rollbackToken(TokenType previous_tt, std::string previous_ti)
{
	assert(("Rollback twice!", !rollback));
	rb_tt = tt;
	rb_ti = ti;
	tt = previous_tt;
	ti = previous_ti;
	rollback = true;
}

void getToken()
{
	if (rollback)
	{
		tt = rb_tt;
		ti = rb_ti;
		rollback = false;
		return;
	}
	while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		getCh();
	if (isalpha(ch) || ch == '_' || ch == '.')
	{
		tt = tkIdent;
		ti = "";
		while (isalpha(ch) || isdigit(ch) || ch == '_' || ch == '.')
		{
			ti += ch;
			getCh();
		}
		for (auto &r: reservedIdentifiers)
			if (ti == r.name)
			{
				tt = r.op;
				return;
			}
	}
	else if (ch == '-' || isdigit(ch))
	{
		ti = ch;
		getCh();
		if (ti == "-" && !isdigit(ch))
		{
			if (ch == '>')
			{
				tt = tkArrow;
				getCh();
				return;
			}
			tt = tkSub;
			return;
		}
		tt = tkNum;
		while (isdigit(ch))
		{
			ti += ch;
			getCh();
		}
	}
	else switch (ch)
	{
	case EOF:
		tt = tkEOF;
		return;

	case '"':
		tt = tkString;
		ti = "";
		getCh();
		while (ch != '"')
		{
			ti += ch;
			if (ch == '\\')
			{
				getCh();
				ti += ch;
			}
			getCh();
		}
		getCh();
		return;

	case '\'':
		getCh();
		tt = tkNum;
		if (ch == '\\')
		{
			getCh();
			switch (ch)
			{
			case 'n':
				ti = std::to_string('\n');
				break;

			case 'r':
				ti = std::to_string('\r');
				break;

			case 't':
				ti = std::to_string('\t');
				break;

			case '\\':
				ti = std::to_string('\\');
				break;

			case '\'':
				ti = std::to_string('\'');
				break;

			default:
				assert(("Unknown character.", 0));
				break;
			}
		}
		else
			ti = std::to_string(ch);
		getCh();
		assert(ch == '\'');
		getCh();
		return;

	case ',':
		tt = tkComma;
		getCh();
		return;

	case ':':
		getCh();
		if (ch == ':')
		{
			tt = tkDoubleColon;
			getCh();
			return;
		}
		tt = tkColon;
		return;

	case ';':
		tt = tkSemicolon;
		getCh();
		return;

	case '&':
		getCh();
		if (ch == '&')
		{
			tt = tkAnd;
			getCh();
		}
		else
			tt = tkBitAnd;
		return;

	case '|':
		getCh();
		if (ch == '|')
		{
			tt = tkOr;
			getCh();
		}
		else
			tt = tkBitOr;
		return;

	case '!':
		getCh();
		if (ch == '=')
		{
			tt = tkNEQ;
			getCh();
		}
		else
			tt = tkNot;
		return;

	case '~':
		tt = tkBitNot;
		getCh();
		return;

	case '<':
		getCh();
		if (ch == '=')
		{
			tt = tkLE;
			getCh();
		}
		else
			tt = tkLT;
		return;

	case '>':
		getCh();
		if (ch == '=')
		{
			tt = tkGE;
			getCh();
		}
		else
			tt = tkGT;
		return;

	case '=':
		getCh();
		if (ch == '=')
		{
			tt = tkEQ;
			getCh();
		}
		else
			tt = tkAssign;
		return;

	case '+':
		tt = tkAdd;
		getCh();
		return;

	case '*':
		tt = tkMul;
		getCh();
		return;

	case '/':
		getCh();
		if (ch == '*')
		{
			getCh();
			for (;;)
			{
				if (ch == '*')
				{
					getCh();
					if (ch == '/')
						break;
				}
				else
					getCh();
			}
			getCh();
			getToken();
		}
		else
			tt = tkDiv;
		return;

	case '%':
		tt = tkRem;
		getCh();
		return;

	case '(':
		tt = tkPLeft;
		getCh();
		return;

	case ')':
		tt = tkPRight;
		getCh();
		return;

	case '[':
		tt = tkSLeft;
		getCh();
		if (ch == ']')
		{
			tt = tkArray;
			getCh();
		}
		return;

	case ']':
		tt = tkSRight;
		getCh();
		return;

	case '{':
		tt = tkBLeft;
		getCh();
		return;

	case '}':
		tt = tkBRight;
		getCh();
		return;

	default:
		assert(("Unrecognized token.", 0));
	}
}

std::string tryGetType(bool *isIdent = nullptr)
{
	if (tt != tkIdent)
		return std::string();
	std::string t = ti, ot = ti;
	for (;;)
	{
		getToken();
		if (tt == tkArray)
			t += "[]";
		else
			break;
	}
	if (isIdent)
		*isIdent = (t == ot);
	return t;
}

std::string getType()
{
	std::string t = tryGetType();
	assert(("Type expected.", !t.empty()));
	return t;
}

#define MODIFIER_PUBLIC		0x0001
#define MODIFIER_PRIVATE	0x0002
#define MODIFIER_PROTECTED	0x0004
#define MODIFIER_STATIC		0x0008
#define MODIFIER_VIRTUAL	0x0010
#define MODIFIER_OVERRIDE	0x0020
#define MODIFIER_ABSTRACT	0x0040
#define MODIFIER_INTERNAL	0x0100
#define MODIFIER_NATIVE		0x0200
#define MODIFIER_INTERFACE	0x1000
#define MODIFIER_AUTOGEN	0x10000
struct ModifierItem
{
	int modifier;
	std::string name;
};

ModifierItem modifierList[] = 
{
	{ MODIFIER_PUBLIC, "public" },
	{ MODIFIER_PRIVATE, "private" },
	{ MODIFIER_PROTECTED, "protected" },
	{ MODIFIER_STATIC, "static" },
	{ MODIFIER_VIRTUAL, "virtual" },
	{ MODIFIER_OVERRIDE, "override" },
	{ MODIFIER_ABSTRACT, "abstract" },
	{ MODIFIER_INTERNAL, "internal" },
	{ MODIFIER_NATIVE, "native" },
	{ MODIFIER_INTERFACE, "interface" },
};

int getModifier()
{
	int modifier = 0;
	bool ok = true;
	while (ok)
	{
		ok = false;
		for (int i = 0; i < ARRAYLENGTH(modifierList); i++)
			if (modifierList[i].name == ti)
			{
				modifier |= modifierList[i].modifier;
				ok = true;
				getToken();
				break;
			}
	}
	return modifier;
}

std::string getModifierString(int modifier)
{
	std::string modifierString = "";
	for (int i = 0; i < ARRAYLENGTH(modifierList); i++)
		if (modifierList[i].modifier & modifier)
			modifierString += modifierList[i].name + " ";
	return modifierString;
}

std::string getOperatorMethodName(TokenType op)
{
	switch (op)
	{
	case tkAdd: return ".opAdd";
	case tkSub: return ".opSub";
	case tkMul: return ".opMul";
	case tkDiv: return ".opDiv";
	case tkRem: return ".opRem";
	case tkLT: return ".opLT";
	case tkLE: return ".opLE";
	case tkGT: return ".opGT";
	case tkGE: return ".opGE";
	case tkEQ: return ".opEQ";
	case tkNEQ: return ".opNEQ";
	case tkArray: return ".opArrayElement";
	}
	assert(("Unsupported operator.", 0));
	return "";
}

#define CC_UNKNOWN			0
#define CC_STDCALL			1
int getCallingConvention()
{
	int cc = 0;
	if (ti == "stdcall")
		cc = CC_STDCALL;
	else
		assert(("Unknown calling convention", false));
	getToken();
	return cc;
}

std::string callingConventionToString(int cc)
{
	if (cc == CC_STDCALL)
		return "stdcall";
	else
	{
		assert(("Unknown calling convention", false));
		return "";
	}
}

/* Parser (Source code -> AST) */

Node *parseObjectAccess();
Node *parseBinop(int priority = 0);
Node *parseList();

Node *parseUnary()
{
	/* FIXME: Special fix */
	if (tt == tkMul) /* '*' */
		tt = tkDeref;
	else if (tt == tkSub) /* '-' */
		tt = tkNeg;
	else if (tt == tkBitAnd) /* '&' */
		tt = tkAddressOf;
	if (tt > tkBinOp && tt <= tkUnaryOp)
	{
		UnaryNode *node = new UnaryNode();
		node->op = tt;
		getToken();
		node->operand = parseObjectAccess();
		return node;
	}
	else if (tt == tkPLeft)
	{
		getToken();
		Node *node = parseList();
		assert(tt == tkPRight);
		getToken();
		return node;
	}
	else if (tt == tkSLeft) /* Type cast */
	{
		getToken();
		TypeCastNode *node = new TypeCastNode();
		node->op = tkTypeCast;
		node->type = getType();
		assert(tt == tkSRight);
		getToken();
		node->operand = parseObjectAccess();
		return node;
	}
	else if (tt == tkThis)
	{
		Node *node = new Node();
		node->op = tkThis;
		getToken();
		return node;
	}
	else if (tt == tkNull)
	{
		Node *node = new Node();
		node->op = tkNull;
		getToken();
		return node;
	}
	else if (tt == tkNew)
	{
		getToken();
		NewNode *node = new NewNode();
		node->type = getType();
		if (tt == tkSLeft)
		{
			node->op = tkNewArray;
			getToken();
			node->parameters.push_back(parseList());
			assert(tt == tkSRight);
			getToken();
		}
		else
		{
			node->op = tkNew;
			assert(tt == tkPLeft);
			getToken();
			if (tt != tkPRight)
				for (;;)
				{
					node->parameters.push_back(parseBinop());
					if (tt == tkPRight)
						break;
					assert(tt == tkComma);
					getToken();
				}
			getToken(); /* ')' */
		}
		return node;
	}
	else if (tt == tkNum)
	{
		Int32Node *node = new Int32Node();
		node->op = tkNum;
		node->intvalue = atoi(ti.c_str());
		getToken();
		return node;
	}
	else if (tt == tkString)
	{
		StringNode *node = new StringNode();
		node->op = tkString;
		node->stringvalue = ti;
		getToken();
		return node;
	}
	else if (tt == tkIdent)
	{
		IdentNode *node = new IdentNode();
		node->op = tkIdent;
		node->ident = ti;
		getToken();
		return node;
	}
	else
	{
		assert(("Unknown unary op.", 0));
		return NULL;
	}
}

Node *parseObjectAccess()
{
	Node *object = parseUnary();
	for (;;)
	{
		if (tt == tkDoubleColon)
		{
			getToken();
			FieldNode *node = new FieldNode();
			node->op = tkField;
			node->object = object;
			node->field = ti;
			getToken();
			object = node;
		}
		else if (tt == tkPLeft)
		{
			getToken();
			CallNode *node = new CallNode();
			node->op = tkCall;
			node->method = object;
			if (tt != tkPRight)
				for (;;)
				{
					node->parameters.push_back(parseBinop());
					if (tt == tkPRight)
						break;
					assert(tt == tkComma);
					getToken();
				}
			getToken(); /* ')' */
			object = node;
		}
		else if (tt == tkSLeft)
		{
			getToken();
			BinNode *node = new BinNode();
			node->op = tkArrayElement;
			node->left = object;
			node->right = parseList();
			assert(tt == tkSRight);
			getToken();
			object = node;
		}
		else
			break;
	}
	return object;
}

/* Parse binary operation with priority higher than 'priority' */
Node *parseBinop(int priority)
{
	Node *left = parseObjectAccess();
	while (tt <= tkBinOp && binop_priority[tt] > priority)
	{
		TokenType op = tt;
		getToken();
		Node *right = parseBinop(binop_priority[op]);
		/* Parse binary operations with higher priority */
		BinNode *node = new BinNode();
		node->op = op;
		node->left = left;
		node->right = right;
		left = node;
	}
	return left;
}

Node *parseList()
{
	Node *node = parseBinop();
	if (tt == tkComma)
	{
		ListNode *ret = new ListNode();
		ret->op = tkList;
		ret->values.push_back(node);
		while (tt == tkComma)
		{
			getToken();
			ret->values.push_back(parseBinop());
		}
		node = ret;
	}
	if (tt == tkAssign)
	{
		getToken();
		BinNode *r = new BinNode();
		r->op = tkAssign;
		r->left = node;
		r->right = parseList();
		return r;
	}
	else
		return node;
}

/* Parse single statement (without semicolon) */
Node *parseSingleStatement()
{
	if (tt != tkIdent)
		return parseList();
	bool isIdent;
	std::string type = tryGetType(&isIdent);
	if (isIdent && tt != tkIdent)
	{
		rollbackToken(tkIdent, type);
		return parseList();
	}
	/* Now we can be sure it is a variable definition statement */
	DefinitionNode *ret = new DefinitionNode();
	ret->op = tkDefinition;
	ret->type = type;
	for (;;)
	{
		assert(tt == tkIdent);
		std::string varname = ti;
		Node *vardefine = nullptr;
		getToken();
		if (tt == tkAssign)
		{
			getToken();
			vardefine = parseBinop();
		}
		ret->variableList.push_back(std::make_pair(varname, vardefine));
		if (tt != tkComma)
			break;
		getToken();
	}
	return ret;
}

/* Parse statement */
Node *parseStatement()
{
	if (tt == tkBLeft)
	{
		MultipleNode *ret = new MultipleNode();
		ret->op = tkMultipleNode;
		getToken();
		while (tt != tkBRight)
			ret->nodes.push_back(parseStatement());
		getToken();
		return ret;
	}
	else if (tt == tkIf)
	{
		IfNode *node = new IfNode();
		node->op = tkIf;
		getToken();
		assert(tt == tkPLeft);
		getToken();
		node->condition = parseList();
		assert(tt == tkPRight);
		getToken();
		node->trueBody = parseStatement();
		if (tt == tkElse)
		{
			getToken();
			node->falseBody = parseStatement();
		}
		else
			node->falseBody = NULL;
		return node;
	}
	else if (tt == tkFor)
	{
		ForNode *node = new ForNode();
		node->op = tkFor;
		getToken();
		assert(tt == tkPLeft);
		getToken();
		node->init = parseSingleStatement();
		assert(tt == tkSemicolon);
		getToken();
		node->condition = parseList();
		assert(tt == tkSemicolon);
		getToken();
		node->update = parseList();
		assert(tt == tkPRight);
		getToken();
		node->body = parseStatement();
		return node;
	}
	else if (tt == tkWhile)
	{
		WhileNode *node = new WhileNode();
		node->op = tkWhile;
		getToken();
		assert(tt == tkPLeft);
		getToken();
		node->condition = parseList();
		assert(tt == tkPRight);
		getToken();
		node->body = parseStatement();
		return node;
	}
	else if (tt == tkDo)
	{
		WhileNode *node = new WhileNode();
		node->op = tkDo;
		getToken();
		node->body = parseStatement();
		assert(tt == tkWhile);
		getToken();
		assert(tt == tkPLeft);
		getToken();
		node->condition = parseList();
		assert(tt == tkPRight);
		getToken();
		assert(tt == tkSemicolon);
		getToken();
		return node;
	}
	else if (tt == tkReturn)
	{
		getToken();
		ReturnNode *node = new ReturnNode();
		node->op = tkReturn;
		if (tt == tkSemicolon)
		{
			ListNode *lnode = new ListNode();
			lnode->op = tkList;
			node->result = lnode;
		}
		else
		{
			node->result = parseList();
			assert(tt == tkSemicolon);
		}
		getToken();
		return node;
	}
	else if (tt == tkThrow)
	{
		getToken();
		ThrowNode *node = new ThrowNode();
		node->op = tkThrow;
		node->exception = parseList();
		assert(tt == tkSemicolon);
		getToken();
		return node;
	}
	else
	{
		Node *node = parseSingleStatement();
		assert(tt == tkSemicolon);
		getToken();
		return node;
	}
}

/* Parse file */
void parseFile()
{
	while (tt == tkIdent && ti == "class")
	{
		Class *classObject = new Class();
		getToken();
		assert(tt == tkIdent);
		classObject->name = ti;
		getToken();
		if (tt == tkColon)
		{
			getToken();
			assert(tt == tkIdent);
			classObject->baseName = ti;
			getToken();
		}
		assert(tt == tkBLeft);
		getToken();
		while (tt != tkBRight)
		{
			int modifier = getModifier();
			if (tt == tkFunction || tt == tkOperator)
			{
				Method *method = new Method();
				method->modifier = modifier;
				if (tt == tkFunction)
				{
					getToken();
					assert(tt == tkIdent);
					method->name = ti;
				}
				else
				{
					assert(("operator must be static", method->modifier & MODIFIER_STATIC));
					getToken();
					method->name = getOperatorMethodName(tt);
				}
				/* Get prototype */
				getToken();
				assert(tt == tkPLeft);
				getToken();
				while (tt != tkPRight)
				{
					/* Parameter types */
					method->paramTypes.push_back(getType());
					if (tt == tkIdent)
					{
						method->paramNames.push_back(ti);
						getToken();
					}
					else
						method->paramNames.push_back("");
					if (tt != tkComma)
						break;
					getToken();
				}
				getToken();
				if (tt == tkArrow)
				{
					/* Return types */
					getToken();
					for (;;)
					{
						method->returnTypes.push_back(getType());
						if (tt == tkIdent)
						{
							method->returnNames.push_back(ti);
							getToken();
						}
						else
							method->returnNames.push_back("");
						if (tt != tkComma)
							break;
						getToken();
					}
				}
				std::string type = "(";
				for (unsigned int i = 0; i < method->paramTypes.size(); i++)
				{
					if (i > 0)
						type += ", ";
					type += method->paramTypes[i];
				}
				type += ")";
				if (method->returnTypes.size())
				{
					type += " -> ";
					for (unsigned int i = 0; i < method->returnTypes.size(); i++)
					{
						if (i > 0)
							type += ", ";
						type += method->returnTypes[i];
					}
				}
				method->type = type;
				if (modifier & MODIFIER_INTERNAL)
				{
					method->body = nullptr;
					getToken(); /* ';' */
				}
				else if (modifier & MODIFIER_NATIVE)
				{
					method->body = nullptr;
					getToken(); /* ';' */
					assert(tt == tkSLeft);
					getToken();
					method->cc = getCallingConvention();
					assert(tt == tkComma);
					getToken();
					assert(tt == tkString);
					method->libraryName = ti;
					getToken();
					assert(tt == tkComma);
					getToken();
					assert(tt == tkString);
					method->originalName = ti;
					getToken();
					assert(tt == tkSRight);
					getToken();
				}
				else
					method->body = parseStatement();
				classObject->methods_vector.push_back(method);
				classObject->methods.insert(std::make_pair(method->name, method));
			}
			else /* Field */
			{
				std::string type = getType();
				for (;;)
				{
					assert(tt == tkIdent);

					Field *field = new Field();
					field->modifier = modifier;
					field->type = type;
					field->name = ti;
					classObject->fields_vector.push_back(field);
					classObject->fields.insert(std::make_pair(field->name, field));
					getToken();
					if (tt == tkSemicolon)
					{
						getToken();
						break;
					}
					assert(tt == tkComma);
					getToken();
				}
			}
		}
		/* Generate constructor if there isn't one */
		if (classObject->methods.find(".ctor") == classObject->methods.end())
		{
			Method *method = new Method();
			method->name = ".ctor";
			method->modifier = MODIFIER_AUTOGEN | MODIFIER_PUBLIC;
			method->type = "()";
			method->body = nullptr;
			classObject->methods_vector.push_back(method);
			classObject->methods.insert(std::make_pair(".ctor", method));
		}
		getToken(); /* '}' */
		classes_vector.push_back(classObject);
		classes.insert(std::make_pair(classObject->name, classObject));
	}
}

/* Compiler (AST -> Bytecode Assembly) */

std::vector<std::unordered_map<std::string, std::pair<int, std::string>>> variables;
int currentLabelId;
Class *currentClass;
Method *currentMethod;
const bool defaultjtrue = false;

std::pair<int, std::string> findVariable(std::string name)
{
	for (auto it = variables.rbegin(); it != variables.rend(); it++)
	{
		auto jt = it->find(name);
		if (jt != it->end())
			return jt->second;
	}
	return std::pair<int, std::string>();
}

void pushSymbolStack()
{
	variables.push_back(std::unordered_map<std::string, std::pair<int, std::string>>());
}

void popSymbolStack()
{
	variables.pop_back();
}

#define VALUE_REGISTER		0x0001
#define VALUE_IMMEDIATE		0x0002
#define VALUE_FIELD			0x0004
#define VALUE_METHOD		0x0008
#define VALUE_ARRAYELEMENT	0x0010
#define VALUE_DEREF			0x0100
#define VALUE_RVALUE		0x1000
#define VALUE_LVALUE		0x2000
#define VALUE_TYPE			0x4000
#define VALUE_JTARGET		0x8000

std::string labelName(unsigned int labelId)
{
	return "_L" + std::to_string(labelId);
}

void setPadding(std::string _padding)
{
	padding = _padding;
}

void addCode(const char *fmt, ...)
{
	const int bufferSize = 4096;
	char buffer[bufferSize];
	va_list va;
	va_start(va, fmt);
	vsprintf_s(buffer, fmt, va);
	code.push_back(padding + std::string(buffer));
}

void addLabel(int labelId)
{
	code.push_back(labelName(labelId) + ":");
}

struct Value
{
	int kind;
	union
	{
		int regid;
		int labelid;
		int intvalue;
	};
	Class *classObject;
	union
	{
		Method *method;
		Field *field;
		int regid2;
	};
	std::string type;
};

bool isPrimitiveType(std::string type)
{
	return type == "uint8" || type == "int8" || type == "uint16" || type == "int16"
		|| type == "uint32" || type == "int32" || type == "uint64" || type == "int64"
		|| type == "char" || type == "int";
}

/* Get instructin suffix for a specific type */
std::string getTypeSuffix(std::string type)
{
	if (type == "uint8")
		return "ub";
	else if (type == "char")
		return "uw";
	else if (type == "int32")
		return "i";
	else if (type == "int")
		return "a";
	else if (type.substr(type.length() - 2) == "[]")
		return "a";
	else
	{
		assert(("Unknown type", classes.find(type) != classes.end()));
		return "a";
	}
}

/* Move register $src to $dst of type 'type' */
void moveRegister(int dst, int src, std::string type)
{
	if (dst != src)
	{
		if (type == "int32" || type == "char" || type == "uint8")
			addCode("ldi\t$%d, $%d", dst, src);
		else
			addCode("lda\t$%d, $%d", dst, src);
	}
}

/* Move value to register */
Value moveValueToRegister(const Value &value, int regid)
{
	if (value.kind & VALUE_REGISTER)
		moveRegister(regid, value.regid, value.type);
	else if (value.kind & VALUE_IMMEDIATE)
	{
		if (value.type == "int32")
			addCode("ldi\t$%d, %d", regid, value.intvalue);
		else if (value.type == "int")
			addCode("ldnull\t$%d", regid);
	}
	else if (value.kind & VALUE_DEREF)
		addCode("ldiuw\t$%d, ($%d)", regid, value.regid);
	else if (value.kind & VALUE_JTARGET)
	{
		addCode("ldi\t$%d, %d", regid, !(int) defaultjtrue);
		addCode("jmp\t%s", labelName(currentLabelId).c_str());
		addLabel(value.labelid);
		addCode("ldi\t$%d, %d", regid, (int) defaultjtrue);
		addLabel(currentLabelId);
		currentLabelId++;
	}
	else if (value.kind & VALUE_FIELD)
	{
		Field *field = value.field;
		if (field->modifier & MODIFIER_STATIC)
			addCode("lds%s\t$%d, %s::%s", getTypeSuffix(field->type).c_str(), regid, value.classObject->name.c_str(), field->name.c_str());
		else
			addCode("ldf%s\t$%d, $%d, %s::%s", getTypeSuffix(field->type).c_str(), regid, value.regid, value.classObject->name.c_str(), field->name.c_str());
	}
	else if (value.kind & VALUE_ARRAYELEMENT)
		addCode("lde%s\t$%d, $%d($%d)", getTypeSuffix(value.type).c_str(), regid, value.regid, value.regid2);
	Value ret;
	ret.kind = VALUE_REGISTER | VALUE_RVALUE;
	ret.regid = regid;
	ret.type = value.type;
	return ret;
}

/* Ensure a value is a register, or move its value to 'regid' */
Value ensureRegister(const Value &value, int regid)
{
	if (value.kind & VALUE_REGISTER)
		return value;
	else
		return moveValueToRegister(value, regid);
}

/* Move value to lvalue, 'freereg' can be equal to 'value' */
void moveValueToLValue(const Value &value, const Value &lvalue, int freereg)
{
	if (lvalue.kind & VALUE_REGISTER)
		moveValueToRegister(value, lvalue.regid);
	else if (lvalue.kind & VALUE_DEREF)
	{
		Value v = ensureRegister(value, freereg);
		addCode("ldiuw\t($%d), $%d", lvalue.regid, v.regid);
	}
	else if (lvalue.kind & VALUE_FIELD)
	{
		Value v = ensureRegister(value, freereg);
		Class *classObject = lvalue.classObject;
		Field *field = lvalue.field;
		if (field->modifier & MODIFIER_STATIC)
			addCode("lds%s\t%s::%s, $%d", getTypeSuffix(field->type).c_str(), classObject->name.c_str(), field->name.c_str(), v.regid);
		else
			addCode("ldf%s\t$%d, %s::%s, $%d", getTypeSuffix(field->type).c_str(), lvalue.regid, classObject->name.c_str(), field->name.c_str(), v.regid);
	}
	else if (lvalue.kind & VALUE_ARRAYELEMENT)
	{
		Value v = ensureRegister(value, freereg);
		addCode("lde%s\t$%d($%d), $%d", getTypeSuffix(lvalue.type).c_str(), lvalue.regid, lvalue.regid2, v.regid);
	}
	else
		assert(("TODO", 0));
}

int getNextFreeRegister(const Value &value, int freereg)
{
	if ((value.kind & VALUE_ARRAYELEMENT) && (value.regid == freereg || value.regid2 == freereg))
	{
		if (value.regid == freereg + 1 || value.regid2 == freereg + 1)
			return freereg + 2;
		else
			return freereg + 1;
	}
	else if (((value.kind & VALUE_REGISTER) || (value.kind & VALUE_DEREF)) && value.regid == freereg)
		return freereg + 1;
	else if (((value.kind & VALUE_FIELD) && !(value.field->modifier & MODIFIER_STATIC)) && value.regid == freereg)
		return freereg + 1;
	else
		return freereg;
}

Value ensureScalar(const std::vector<Value> &values)
{
	assert(("Empty list.", values.size() > 0));
	return values[0];
}

std::vector<Value> scalar(const Value &value)
{
	std::vector<Value> ret;
	ret.push_back(value);
	return ret;
}

int fixJumpTarget(int jtarget)
{
	if (jtarget == 0)
		return currentLabelId++;
	else
		return jtarget;
}

Field *findField(Class *classObject, std::string name, Class **fieldClassObject)
{
	*fieldClassObject = classObject;
	for (;;)
	{
		auto it = (*fieldClassObject)->fields.find(name);
		if (it != (*fieldClassObject)->fields.end())
			return it->second;
		if ((*fieldClassObject)->baseName == "")
			return nullptr;
		*fieldClassObject = classes[(*fieldClassObject)->baseName];
	}
}

std::vector<Value> compileList(Node *node, int freereg, int targetreg = -1);
std::vector<Value> compileCall(Node *node, int freereg)
{
	CallNode *cnode = (CallNode *) node;
	Value methodValue = ensureScalar(compileList(cnode->method, freereg));
	assert(methodValue.kind & VALUE_METHOD);
	Class *classObject = methodValue.classObject;
	Method *method = methodValue.method;

	/* Push 'this' */
	int firstreg = freereg;
	if (!(method->modifier & MODIFIER_STATIC))
	{
		moveRegister(freereg, methodValue.regid, methodValue.type);
		firstreg = freereg + 1;
	}
	else
		firstreg = freereg;

	/* Push remaining parameters */
	assert(cnode->parameters.size() == method->paramTypes.size());
	for (unsigned int i = 0; i < method->paramTypes.size(); i++)
	{
		Value value = ensureScalar(compileList(cnode->parameters[i], firstreg + i));
		assert(value.type == method->paramTypes[i]);
		moveValueToRegister(value, firstreg + i);
	}
	addCode("call\t$%d, %s::%s%s", freereg, classObject->name.c_str(), method->name.c_str(), method->type.c_str());
	std::vector<Value> ret;
	for (unsigned int i = 0; i < method->returnTypes.size(); i++)
	{
		Value value;
		value.kind = VALUE_REGISTER | VALUE_RVALUE;
		value.regid = freereg + i;
		value.type = method->returnTypes[i];
		ret.push_back(value);
	}
	return ret;
}

Value compileCustomBinaryOperator(TokenType op, const Value &left, const Value &right, int firstreg)
{
	/* We only try finding the method in left type */
	moveValueToRegister(left, firstreg);
	moveValueToRegister(right, firstreg + 1);

	std::string methodName = getOperatorMethodName(op);
	Method *method = classes[left.type]->methods[methodName];
	assert(method->paramTypes.size() == 2);
	assert(method->returnTypes.size() == 1);
	assert(method->paramTypes[0] == left.type);
	assert(method->paramTypes[1] == right.type);

	addCode("call\t$%d, %s::%s%s", firstreg, left.type.c_str(), methodName.c_str(), method->type.c_str());
	Value ret;
	ret.kind = VALUE_REGISTER | VALUE_RVALUE;
	ret.regid = firstreg;
	ret.type = method->returnTypes[0];
	return ret;
}

Value compileExpression(Node *node, int freereg, int targetreg, int jtarget = 0, bool jtrue = defaultjtrue)
{
	if (node->op <= tkBinArith)
	{
		BinNode *bnode = (BinNode *) node;
		Value left = ensureRegister(compileExpression(bnode->left, freereg, freereg), freereg);
		Value right = compileExpression(bnode->right, freereg + 1, freereg + 1);
		const char *inst = NULL;
		if (left.type == right.type && left.type == "int32")
		{
			switch (bnode->op)
			{
			case tkAdd: inst = "addi"; break;
			case tkSub: inst = "subi"; break;
			case tkMul: inst = "muli"; break;
			case tkDiv: inst = "divi"; break;
			case tkRem: inst = "remi"; break;
			case tkBitAnd: inst = "andi"; break;
			case tkBitOr: inst = "ori"; break;
			}
		}
		else if (left.type == "int" && right.type == "int32")
		{
			right = ensureRegister(right, freereg + 1);
			switch (bnode->op)
			{
			case tkAdd: inst = "addni"; break;
			case tkSub: inst = "subni"; break;
			}
		}
		else
			return compileCustomBinaryOperator(bnode->op, left, right, freereg);

		if ((right.kind & VALUE_IMMEDIATE) && right.type == "int32")
			addCode("%s\t$%d, $%d, %d", inst, targetreg, left.regid, right.intvalue);
		else
			addCode("%s\t$%d, $%d, $%d", inst, targetreg, left.regid, ensureRegister(right, freereg + 1).regid);
		Value ret;
		ret.kind = VALUE_REGISTER | VALUE_RVALUE;
		ret.regid = targetreg;
		ret.type = left.type;
		return ret;
	}
	else if (node->op <= tkBinComparison)
	{
		jtarget = fixJumpTarget(jtarget);

		BinNode *bnode = (BinNode *) node;
		Value left = ensureRegister(compileExpression(bnode->left, freereg, freereg), freereg);
		Value right = compileExpression(bnode->right, freereg + 1, freereg + 1);
		const char *inst = NULL;
		if (left.type == right.type && (left.type == "int32" || left.type == "char"))
		{
			if (jtrue) /* jump on true */
			{
				switch (bnode->op)
				{
				case tkLT: inst = "lt"; break;
				case tkLE: inst = "le"; break;
				case tkGT: inst = "gt"; break;
				case tkGE: inst = "ge"; break;
				case tkEQ: inst = "eq"; break;
				case tkNEQ: inst = "neq"; break;
				}
			}
			else /* jump on false */
			{
				switch (bnode->op)
				{
				case tkLT: inst = "ge"; break;
				case tkLE: inst = "gt"; break;
				case tkGT: inst = "le"; break;
				case tkGE: inst = "lt"; break;
				case tkEQ: inst = "neq"; break;
				case tkNEQ: inst = "eq"; break;
				}
			}
			if ((right.kind & VALUE_IMMEDIATE) && right.intvalue == 0)
				addCode("j%szi\t$%d, %s", inst, left.regid, labelName(jtarget).c_str());
			else
			{
				right = ensureRegister(right, freereg + 1);
				addCode("j%si\t$%d, $%d, %s", inst, left.regid, right.regid, labelName(jtarget).c_str());
			}
		}
		else if ((!isPrimitiveType(left.type) || left.type == "int") && right.type == "int")
		{
			assert(right.kind & VALUE_IMMEDIATE);
			if (jtrue) /* jump on true */
				if (bnode->op == tkEQ)
					inst = "jn";
				else
				{
					assert(bnode->op == tkNEQ);
					inst = "jnn";
				}
			else /* jump on false */
				if (bnode->op == tkEQ)
					inst = "jnn";
				else
				{
					assert(bnode->op == tkNEQ);
					inst = "jn";
				}
			addCode("%s\t$%d, %s", inst, left.regid, labelName(jtarget).c_str());
		}
		else
			assert(("Unhandled jcc case.", 0));
		Value ret;
		ret.kind = VALUE_JTARGET;
		ret.labelid = jtarget;
		return ret;
	}
	else if (node->op <= tkBinLogic)
	{
		jtarget = fixJumpTarget(jtarget);

		BinNode *bnode = (BinNode *) node;
		bool leftjtrue, rightjtrue;
		int leftjtarget, rightjtarget;
		/* Calculate jump target of left/right */
		if (node->op == tkAnd)
		{
			if (jtrue)
			{
				leftjtrue = false;
				leftjtarget = currentLabelId++;
				rightjtrue = true;
				rightjtarget = jtarget;
			}
			else
			{
				leftjtrue = false;
				rightjtrue = false;
				leftjtarget = rightjtarget = jtarget;
			}
		}
		else /* tkOr */
		{
			if (jtrue)
			{
				leftjtrue = true;
				rightjtrue = true;
				leftjtarget = rightjtarget = jtarget;
			}
			else
			{
				leftjtrue = true;
				leftjtarget = currentLabelId++;
				rightjtrue = false;
				rightjtarget = jtarget;
			}
		}
		Value left = compileExpression(bnode->left, freereg, freereg, leftjtarget, leftjtrue);
		assert(left.kind & VALUE_JTARGET);
		Value right = compileExpression(bnode->right, freereg, freereg, rightjtarget, rightjtrue);
		assert(right.kind & VALUE_JTARGET);
		if (leftjtarget != jtarget)
			addLabel(leftjtarget);

		Value ret;
		ret.kind = VALUE_JTARGET;
		ret.labelid = jtarget;
		ret.type = "int32";
		return ret;
	}
	else if (node->op <= tkUnaryOp)
	{
		UnaryNode *unode = (UnaryNode *) node;
		if (node->op == tkDeref)
		{
			/* Dereference */
			Value value = ensureRegister(compileExpression(unode->operand, freereg, freereg), freereg);
			assert(value.type == "int");
			Value ret;
			ret.kind = VALUE_DEREF | VALUE_LVALUE;
			ret.regid = value.regid;
			ret.type = "char";
			return ret;
		}
		else if (node->op == tkAddressOf)
		{
			Value value = compileExpression(unode->operand, freereg, targetreg);
			assert(value.kind & VALUE_LVALUE);
			if (value.kind & VALUE_REGISTER)
				addCode("lar\t$%d, $%d", targetreg, value.regid);
			else if (value.kind & VALUE_FIELD)
			{
				Class *classObject = value.classObject;
				Field *field = value.field;
				if (field->modifier & MODIFIER_STATIC)
					addCode("las\t$%d, %s::%s", targetreg, classObject->name.c_str(), field->name.c_str());
				else
					addCode("laf\t$%d, $%d, %s::%s", targetreg, value.regid, classObject->name.c_str(), field->name.c_str());
			}
			else if (value.kind & VALUE_ARRAYELEMENT)
				addCode("lae\t$%d, $%d($%d)", targetreg, value.regid, value.regid2);
			else
				assert(("Unhandled case.", 0));
			Value ret;
			ret.kind = VALUE_REGISTER | VALUE_RVALUE;
			ret.regid = targetreg;
			ret.type = "int";
			return ret;
		}
		else
		{
			Value value = ensureRegister(compileExpression(unode->operand, freereg, targetreg), targetreg);
			assert(value.type == "int32");
			const char *inst = nullptr;
			switch (node->op)
			{
			case tkNeg: inst = "negi"; break;
			case tkBitNot: inst = "noti"; break;
			}
			addCode("%s\t$%d, $%d", inst, targetreg, value.regid);
			Value ret;
			ret.kind = VALUE_REGISTER | VALUE_RVALUE;
			ret.regid = targetreg;
			ret.type = value.type;
			return ret;
		}
	}
	else if (node->op == tkCall)
		return ensureScalar(compileCall(node, freereg));
	else if (node->op == tkThis)
	{
		Value ret;
		ret.kind = VALUE_REGISTER | VALUE_RVALUE;
		ret.regid = 0;
		ret.type = currentClass->name;
		return ret;
	}
	else if (node->op == tkNew)
	{
		NewNode *nnode = (NewNode *) node;
		Class *classObject = classes[nnode->type];
		Method *method = classObject->methods[".ctor"];
		assert(method->paramTypes.size() == nnode->parameters.size());
		for (unsigned int i = 0; i < nnode->parameters.size(); i++)
		{
			Value value = ensureScalar(compileList(nnode->parameters[i], freereg + 2 + i));
			assert(value.type == method->paramTypes[i]);
			moveValueToRegister(value, freereg + 2 + i);
		}
		addCode("new\t$%d, %s::%s%s", freereg, classObject->name.c_str(), method->name.c_str(), method->type.c_str());
		Value ret;
		ret.kind = VALUE_REGISTER | VALUE_RVALUE;
		ret.regid = freereg;
		ret.type = classObject->name;
		return ret;
	}
	else if (node->op == tkNewArray)
	{
		NewNode *nnode = (NewNode *) node;
		assert(nnode->parameters.size() == 1);
		
		Value value = ensureRegister(ensureScalar(compileList(nnode->parameters[0], freereg)), freereg);
		addCode("newarr\t$%d, %s($%d)", targetreg, nnode->type.c_str(), value.regid);
		Value ret;
		ret.kind = VALUE_REGISTER | VALUE_RVALUE;
		ret.regid = targetreg;
		ret.type = nnode->type + "[]";
		return ret;
	}
	else if (node->op == tkField)
	{
		FieldNode *fnode = (FieldNode *) node;

		Value object = compileExpression(fnode->object, freereg, freereg);
		if (object.type.substr(object.type.length() - 2) == "[]")
		{
			assert(fnode->field == "Length");
			object = ensureRegister(object, freereg);
			addCode("ldlen\t$%d, $%d", targetreg, object.regid);
			Value ret;
			ret.kind = VALUE_REGISTER;
			ret.regid = targetreg;
			ret.type = "int32";
			return ret;
		}
		Value ret;
		if (object.kind & VALUE_TYPE)
			ret.classObject = object.classObject;
		else
		{
			object = ensureRegister(object, freereg);
			ret.regid = object.regid;
			ret.classObject = classes[object.type];
		}
		Class *fieldClassObject;
		Field *field = findField(ret.classObject, fnode->field, &fieldClassObject);
		if (field)
		{
			/* Field */
			ret.kind = VALUE_FIELD | VALUE_LVALUE;
			ret.classObject = fieldClassObject;
			ret.field = field;
			ret.type = ret.field->type;
		}
		else
		{
			/* Method */
			auto it = ret.classObject->methods.find(fnode->field);
			assert(it != ret.classObject->methods.end());
			ret.kind = VALUE_METHOD;
			ret.method = it->second;
		}
		return ret;
	}
	else if (node->op == tkTypeCast)
	{
		TypeCastNode *tnode = (TypeCastNode *) node;
		Value value = ensureRegister(compileExpression(tnode->operand, freereg, targetreg), targetreg);
		if (isPrimitiveType(value.type))
		{
			/* No conversion from/to primitive type now */
			assert(isPrimitiveType(tnode->type));
			value.type = tnode->type;
			return value;
		}
		else
		{
			addCode("cast\t$%d, $%d, %s", targetreg, value.regid, tnode->type.c_str());
			Value ret;
			ret.kind = VALUE_REGISTER | VALUE_RVALUE;
			ret.regid = targetreg;
			ret.type = tnode->type;
			return ret;
		}
	}
	else if (node->op == tkArrayElement)
	{
		BinNode *bnode = (BinNode *) node;
		Value left = ensureRegister(ensureScalar(compileList(bnode->left, freereg)), freereg);
		if (left.type.length() > 2 && left.type.substr(left.type.length() - 2) == "[]")
		{
			freereg = getNextFreeRegister(left, freereg);
			Value right = ensureRegister(ensureScalar(compileList(bnode->right, freereg)), freereg);
			Value ret;
			ret.kind = VALUE_ARRAYELEMENT | VALUE_LVALUE;
			ret.regid = left.regid;
			ret.regid2 = right.regid;
			ret.type = left.type.substr(0, left.type.length() - 2);
			return ret;
		}
		else
		{
			Value right = ensureRegister(ensureScalar(compileList(bnode->right, freereg + 1)), freereg + 1);
			return compileCustomBinaryOperator(tkArray, left, right, freereg);
		}
	}
	else if (node->op == tkAssign)
		return ensureScalar(compileList(node, freereg));
	else if (node->op == tkIdent)
	{
		IdentNode *inode = (IdentNode *) node;
		auto it = classes.find(inode->ident);
		if (it != classes.end())
		{
			Value ret;
			ret.kind = VALUE_TYPE;
			ret.classObject = it->second;
			ret.type = inode->ident;
			return ret;
		}
		else
		{
			std::pair<int, std::string> var = findVariable(inode->ident);
			if (var != std::pair<int, std::string>())
			{
				Value ret;
				ret.kind = VALUE_REGISTER | VALUE_LVALUE;
				ret.regid = var.first;
				ret.type = var.second;
				return ret;
			}
			else
			{
				auto it = currentClass->fields.find(inode->ident);
				if (it != currentClass->fields.end())
				{
					Value ret;
					ret.kind = VALUE_FIELD | VALUE_LVALUE;
					ret.regid = 0;
					ret.classObject = currentClass;
					ret.field = it->second;
					ret.type = ret.field->type;
					return ret;
				}
				else
				{
					auto it = currentClass->methods.find(inode->ident);
					if (it != currentClass->methods.end())
					{
						Value ret;
						ret.regid = 0;
						ret.classObject = currentClass;
						ret.kind = VALUE_METHOD;
						ret.method = it->second;
						return ret;
					}
					else
						assert(("Unknown identifier.", 0));
				}
			}
		}
	}
	else if (node->op == tkNum)
	{
		Int32Node *inode = (Int32Node *) node;
		Value ret;
		ret.kind = VALUE_IMMEDIATE | VALUE_RVALUE;
		ret.intvalue = inode->intvalue;
		ret.type = "int32";
		return ret;
	}
	else if (node->op == tkString)
	{
		StringNode *snode = (StringNode *) node;
		addCode("ldstr\t$%d, \"%s\"", targetreg, snode->stringvalue.c_str());
		Value ret;
		ret.kind = VALUE_REGISTER | VALUE_RVALUE;
		ret.regid = targetreg;
		ret.type = "Core.String";
		return ret;
	}
	else if (node->op == tkNull)
	{
		Value ret;
		ret.kind = VALUE_IMMEDIATE | VALUE_RVALUE;
		ret.type = "int";
		return ret;
	}
	assert(("Unhandled case", 0));
	return Value();
}

/* Call node is the only 'expression'-like node that can return multiple values (or zero).
   So don't bother mentioning it many times here */
/* Save last scalar value to 'targetreg' and other values to stack position if 'targetreg' != -1 */
std::vector<Value> compileList(Node *node, int freereg, int targetreg)
{
	if (node->op == tkList)
	{
		ListNode *lnode = (ListNode *) node;
		std::vector<Value> ret;
		for (unsigned int i = 0; i < lnode->values.size(); i++)
		{
			Node *node = lnode->values[i];
			if (node->op == tkCall)
			{
				std::vector<Value> r = compileCall(node, freereg + ret.size());
				ret.insert(ret.end(), r.begin(), r.end());
			}
			else if (targetreg != -1)
			{
				if (i + 1 == lnode->values.size()) /* Last scalar */
					ret.push_back(ensureRegister(compileExpression(node, freereg + ret.size(), targetreg), targetreg));
				else /* Ensure value is a register at stack position */
					ret.push_back(moveValueToRegister(compileExpression(node, freereg + ret.size(), freereg + ret.size()), freereg + ret.size()));
			}
			else
				ret.push_back(ensureRegister(compileExpression(node, freereg + ret.size(), freereg + ret.size()), freereg + ret.size()));
		}
		return ret;
	}
	else if (node->op == tkAssign)
	{
		BinNode *bnode = (BinNode *) node;
		if (bnode->left->op != tkList)
		{
			BinNode *bnode = (BinNode *) node;
			/* Single operand assign */
			Value left = compileExpression(bnode->left, freereg, freereg);
			assert(left.kind & VALUE_LVALUE);
			freereg = getNextFreeRegister(left, freereg);
			int targetreg = freereg;
			if (left.kind & VALUE_REGISTER)
				targetreg = left.regid;
			Value right = compileExpression(bnode->right, freereg, targetreg);
			moveValueToLValue(right, left, freereg);
			return scalar(left);
		}
		else
		{
			std::vector<Value> left;
			/* Ensure lvalue */
			ListNode *lbnode = (ListNode *) bnode->left;
			for (Node *node: lbnode->values)
			{
				Value value = compileExpression(node, freereg, freereg);
				freereg = getNextFreeRegister(value, freereg);
				left.push_back(value);
				assert(value.kind & VALUE_LVALUE);
			}

			/* Optimize: if last element of 'left' is a register, directly store corresponding result in 'right'
			             to that register */
			if (left.back().kind & VALUE_REGISTER)
				targetreg = left.back().regid;
			std::vector<Value> right = compileList(bnode->right, freereg, targetreg);
			assert(("List size mismatch.", left.size() == right.size() || left.size() == 1));
			/* Ensure all elements in 'right' is in the stack, also, the last one does not need to be */
			for (unsigned int i = 0; i + 1 < left.size(); i++)
				if (!(right[i].kind & VALUE_IMMEDIATE)) /* Immediates does not need to be in the stack */
					right[i] = moveValueToRegister(right[i], freereg + i);
			/* Move results to left, note the last one must be copied first, due to the optimization above */
			for (int i = left.size() - 1; i >= 0; i--)
				moveValueToLValue(right[i], left[i], freereg + i);
			return left;
		}
	}
	else if (node->op == tkCall)
		return compileCall(node, freereg);
	else /* Scalar */
	{
		if (targetreg != -1)
			return scalar(compileExpression(node, freereg, targetreg));
		else
			return scalar(compileExpression(node, freereg, freereg));
	}
}

/* Returns whether the statement block is fully closed (has return statements in all possible control flows) */
bool compileStatementBlock(Node *node, int freereg);
bool compileStatement(Node *node, int &freereg)
{
	if (node->op == tkMultipleNode)
		return compileStatementBlock(node, freereg);
	else if (node->op == tkDefinition)
	{
		DefinitionNode *dnode = (DefinitionNode *) node;
		for (auto variable: dnode->variableList)
		{
			int reg = freereg;
			variables.back()[variable.first] = std::make_pair(reg, dnode->type);
			if (variable.second) /* definition block */
			{
				Value value = compileExpression(variable.second, freereg, reg);
				moveValueToRegister(value, reg);
			}
			freereg = freereg + 1;
		}
		return false;
	}
	else if (node->op == tkIf)
	{
		IfNode *inode = (IfNode *) node;
		if (!inode->falseBody)
		{
			/* If - Then */
			int falseTarget = currentLabelId++;
			Value value = compileExpression(inode->condition, freereg, freereg, falseTarget, false);
			assert(value.kind & VALUE_JTARGET);
			compileStatementBlock(inode->trueBody, freereg);
			addLabel(falseTarget);
			return false;
		}
		else
		{
			/* If - Then - Else */
			int falseTarget = currentLabelId++;
			int endTarget = currentLabelId++;
			Value value = compileExpression(inode->condition, freereg, freereg, falseTarget, false);
			assert(value.kind & VALUE_JTARGET);
			bool thenClosed, elseClosed;
			thenClosed = compileStatementBlock(inode->trueBody, freereg);
			if (!thenClosed)
				addCode("jmp\t%s", labelName(endTarget).c_str());
			addLabel(falseTarget);
			elseClosed = compileStatementBlock(inode->falseBody, freereg);
			if (!thenClosed)
				addLabel(endTarget);
			return thenClosed & elseClosed;
		}
	}
	else if (node->op == tkFor)
	{
		pushSymbolStack();
		ForNode *fnode = (ForNode *) node;
		int bodyTarget = currentLabelId++;
		int conditionTarget = currentLabelId++;
		int _freereg = freereg;
		compileStatement(fnode->init, _freereg);
		addCode("jmp\t%s", labelName(conditionTarget).c_str());
		addLabel(bodyTarget);
		compileStatementBlock(fnode->body, _freereg);
		compileList(fnode->update, _freereg);
		addLabel(conditionTarget);
		Value value = compileExpression(fnode->condition, _freereg, _freereg, bodyTarget, true);
		assert(value.kind == VALUE_JTARGET);
		popSymbolStack();
		return false;
	}
	else if (node->op == tkWhile)
	{
		WhileNode *wnode = (WhileNode *) node;
		int bodyTarget = currentLabelId++;
		int conditionTarget = currentLabelId++;
		addCode("jmp\t%s", labelName(conditionTarget).c_str());
		addLabel(bodyTarget);
		compileStatementBlock(wnode->body, freereg);
		addLabel(conditionTarget);
		Value value = compileExpression(wnode->condition, freereg, freereg, bodyTarget, true);
		assert(value.kind == VALUE_JTARGET);
		return false;
	}
	else if (node->op == tkDo)
	{
		WhileNode *wnode = (WhileNode *) node;
		int bodyTarget = currentLabelId++;
		addLabel(bodyTarget);
		compileStatementBlock(wnode->body, freereg);
		Value value = compileExpression(wnode->condition, freereg, freereg, bodyTarget, true);
		assert(value.kind == VALUE_JTARGET);
		return false;
	}
	else if (node->op == tkReturn)
	{
		ReturnNode *rnode = (ReturnNode *) node;
		std::vector<Value> values = compileList(rnode->result, freereg);
		unsigned int returnCount = currentMethod->returnNames.size();
		assert(values.size() == returnCount || returnCount == 1);
		if (returnCount == 0)
			addCode("ret");
		else if (returnCount == 1)
			addCode("ret\t$%d", ensureRegister(values[0], freereg).regid);
		else
		{
			/* Ensure all values are in register stack */
			for (unsigned int i = 0; i < returnCount; i++)
				moveValueToRegister(values[i], freereg + i);
			addCode("ret\t$%d, $%d", freereg, freereg + returnCount - 1);
		}
		return true;
	}
	else if (node->op == tkThrow)
	{
		ThrowNode *tnode = (ThrowNode *) node;
		Value value = ensureRegister(ensureScalar(compileList(tnode->exception, freereg)), freereg);
		addCode("throw\t$%d", value.regid);
		return true;
	}
	else
	{
		compileList(node, freereg);
		return false;
	}
}

/* Statement block is a block with a new variable scoping */
bool compileStatementBlock(Node *node, int freereg)
{
	pushSymbolStack();
	bool closed = false;
	if (node->op == tkMultipleNode)
	{
		MultipleNode *mnode = (MultipleNode *) node;
		int _freereg = freereg;
		for (Node *node: mnode->nodes)
			closed |= compileStatement(node, _freereg);
	}
	else
		closed = compileStatement(node, freereg);
	popSymbolStack();
	return closed;
}

void compile()
{
	for (Class *classObject: classes_vector)
	{
		printf("Compiling class %s...\n", classObject->name.c_str());
		if (classObject->baseName.empty())
			addCode(".class %s", classObject->name.c_str());
		else
			addCode(".class %s: %s", classObject->name.c_str(), classObject->baseName.c_str());
		addCode("{");
		setPadding("\t");

		currentClass = classObject;
		for (Field *field: classObject->fields_vector)
			addCode(".field %s%s %s", getModifierString(field->modifier).c_str(), field->type.c_str(), field->name.c_str());
		for (Method *method: classObject->methods_vector)
		{
			printf("  ::%s\n", method->name.c_str());
			addCode(".method %sfunction %s%s", getModifierString(method->modifier).c_str(), method->name.c_str(), method->type.c_str());
			if (method->modifier & MODIFIER_INTERNAL)
				/* NOP */;
			else if (method->modifier & MODIFIER_NATIVE)
				addCode(
					"[%s, \"%s\", \"%s\"]",
					callingConventionToString(method->cc).c_str(),
					method->libraryName.c_str(),
					method->originalName.c_str()
				);
			else if (method->modifier & MODIFIER_AUTOGEN)
			{
				/* Automatically generated constructor */
				addCode("{");
				addCode("\tret");
				addCode("}");
			}
			else
			{
				addCode("{");
				setPadding("\t\t");
				pushSymbolStack();

				int freereg = 0;
				/* Parameter slot for 'this' */
				if (!(method->modifier & MODIFIER_STATIC))
					freereg++;
				for (unsigned int i = 0; i < method->paramNames.size(); i++)
					if (!method->paramNames[i].empty())
						variables[0].insert(std::make_pair(method->paramNames[i], std::make_pair(freereg + i, method->paramTypes[i])));
				freereg += method->paramNames.size();

				currentLabelId = 1;
				currentMethod = method;
				if (!compileStatementBlock(method->body, freereg))
				{
					/* Unclosed statement block, insert return statement manually */
					assert(method->returnNames.size() == 0);
					addCode("ret");
				}

				popSymbolStack();
				setPadding("\t");
				addCode("}");
			}
		}
		setPadding("");
		addCode("}");
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
		assert(("ERROR", 0));
	for (int i = 1; i < argc - 1; i++)
	{
		printf("Parsing %s...\n", argv[i]);
		freopen(argv[i], "r", stdin);
		getCh();
		getToken();
		parseFile();
	}
	compile();
	puts("Compilation succeeded.");
	freopen(argv[argc - 1], "w", stdout);
	for (std::string line: code)
		puts(line.c_str());
	return 0;
}
