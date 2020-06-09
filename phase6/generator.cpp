/*
 * File:	generator.cpp
 *
 * Description:	This file contains the public and member function
 *		definitions for the code generator for Simple C.
 *
 *		Extra functionality:
 *		- putting all the global declarations at the end
 */

# include <cassert>
# include <iostream>
# include "generator.h"
# include "machine.h"
# include "Tree.h"

# define FP(expr) ((expr)->type().isReal())
# define BYTE(expr) ((expr)->type().size() == 1)

using namespace std;

static int offset;
static unsigned max_args;


/*
 * Function:	align (private)
 *
 * Description:	Return the number of bytes necessary to align the given
 *		offset on the stack.
 */

static int align(int offset)
{
    if (offset % STACK_ALIGNMENT == 0)
	return 0;

    return STACK_ALIGNMENT - (abs(offset) % STACK_ALIGNMENT);
}


/*
 * Function:	operator << (private)
 *
 * Description:	Convenience function for writing the operand of an
 *		expression using the output stream operator.
 */

static ostream &operator <<(ostream &ostr, Expression *expr)
{
    expr->operand(ostr);
    return ostr;
}


/*
 * Function:	Expression::operand
 *
 * Description:	Write an expression as an operator to the specified stream.
 */

void Expression::operand(ostream &ostr) const
{
    assert(offset != 0);
    ostr << offset << "(%ebp)";
}


/*
 * Function:	Identifier::operand
 *
 * Description:	Write an identifier as an operand to the specified stream.
 */

void Identifier::operand(ostream &ostr) const
{
    if (_symbol->offset == 0)
	ostr << global_prefix << _symbol->name();
    else
	ostr << _symbol->offset << "(%ebp)";
}


/*
 * Function:	Integer::operand
 *
 * Description:	Write an integer as an operand to the specified stream.
 */

void Integer::operand(ostream &ostr) const
{
    ostr << "$" << _value;
}


/*
 * Function:	Call::generate
 *
 * Description:	Generate code for a function call expression.
 */

void Call::generate()
{
    unsigned offset, size;


    /* Generate code for all arguments first. */

    for (auto arg : _args)
	arg->generate();


    /* Move the arguments onto the stack. */

    offset = 0;

    for (auto arg : _args) {
	if (FP(arg)) {
	    cout << "\tfldl\t" << arg << endl;
	    cout << "\tfstpl\t" << offset << "(%esp)" << endl;
	} else {
	    cout << "\tmovl\t" << arg << ", %eax" << endl;
	    cout << "\tmovl\t%eax, " << offset << "(%esp)" << endl;
	}

	size = arg->type().size();
	offset += size;
    }

    if (offset > max_args)
	max_args = offset;


    /* Make the function call. */

    cout << "\tcall\t" << global_prefix << _id->name() << endl;


    /* Save the return value */

# if 0
    assigntemp(this);

    if (FP(this))
	cout << "\tfstpl\t" << this << endl;
    else
	cout << "\tmovl\t%eax, " << this << endl;
# endif
}


/*
 * Function:	Block::generate
 *
 * Description:	Generate code for this block, which simply means we
 *		generate code for each statement within the block.
 */

void Block::generate()
{
    for (auto stmt : _stmts)
	stmt->generate();
}


/*
 * Function:	Function::generate
 */

void Function::generate()
{
    max_args = 0;
    offset = SIZEOF_REG * 2;
    allocate(offset);


    /* Generate our prologue. */

    cout << global_prefix << _id->name() << ":" << endl;
    cout << "\tpushl\t%ebp" << endl;
    cout << "\tmovl\t%esp, %ebp" << endl;
    cout << "\tsubl\t$" << _id->name() << ".size, %esp" << endl;


    /* Generate the body of this function. */

    _body->generate();


    /* Compute the proper stack frame size. */

    offset -= max_args;
    offset -= align(offset - SIZEOF_REG * 2);


    /* Generate our epilogue. */

    cout << "\tmovl\t%ebp, %esp" << endl;
    cout << "\tpopl\t%ebp" << endl;
    cout << "\tret" << endl << endl;

    cout << "\t.set\t" << _id->name() << ".size, " << -offset << endl;
    cout << "\t.globl\t" << global_prefix << _id->name() << endl << endl;
}


/*
 * Function:	generateGlobals
 *
 * Description:	Generate code for any global variable declarations.
 */

void generateGlobals(Scope *scope)
{
    const Symbols &symbols = scope->symbols();

    for (auto symbol : symbols)
	if (!symbol->type().isFunction()) {
	    cout << "\t.comm\t" << global_prefix << symbol->name() << ", ";
	    cout << symbol->type().size() << endl;
	}
}


/*
 * Function:	Assignment::generate
 *
 * Description:	Generate code for an assignment statement.
 *
 *		NOT FINISHED: Only works if the right-hand side is an
 *		integer literal and the left-hand side is an integer
 *		scalar.
 */

void Assignment::generate()
{
    cout << "\tmovl\t" << _right << ", " << _left << endl;
}
